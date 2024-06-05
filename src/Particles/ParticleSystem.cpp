#include "ParticleSystem.h"

#include <algorithm>
#include <cstdlib>

#include "../GL/Framebuffer.h"
#include "../GL/Geometry.h"
#include "../GL/Shader.h"
#include "../GL/StateManager.h"
#include "../GL/Texture.h"
#include "../Loader/Loader.h"

struct Particle {
    // 3f position, 1f rotation
    glm::vec4 position_rotation;
    // 3f velocity, 1f rotation speed
    glm::vec4 velocity_revolutions;
    // 1f drag, 1f gravity, 1f random
    glm::vec4 drag_gravity_rand;
    // 2f size, 1f life remaining, 1f life max
    glm::vec4 size_life;
    int emitter;
};

// integer divide x / y but round up instead of truncate
#define DIV_CEIL(x, y) ((x + y - 1) / y)

struct EmitterShaderValues {
    glm::ivec4 index_length;
    glm::vec4 gravity;
};

void ParticleMaterial::destroy() {
    delete sprite;
    sprite = nullptr;
    delete tint;
    tint = nullptr;
    delete scale;
    scale = nullptr;
}

ParticleSystem::ParticleSystem(int capacity) {
    this->capacity_ = capacity;
    particleBuffer_ = new gl::Buffer();
    particleBuffer_->setDebugLabel("particle_system/particle_buffer");
    particleBuffer_->allocateEmpty(sizeof(Particle) * capacity, 0);

    freeBuffer_ = new gl::Buffer();
    freeBuffer_->setDebugLabel("particle_system/free_buffer");
    freeBuffer_->allocateEmpty(capacity_ * sizeof(GLuint), GL_DYNAMIC_STORAGE_BIT);

    freeHeadsBuffer_ = new gl::Buffer();
    freeHeadsBuffer_->setDebugLabel("particle_system/free_heads_buffer");
    freeHeadsBuffer_->allocateEmpty(MAX_EMITTERS * sizeof(GLint), GL_DYNAMIC_STORAGE_BIT);

    emitterBuffer_ = new gl::Buffer();
    emitterBuffer_->setDebugLabel("particle_system/emitter_buffer");
    emitterBuffer_->allocateEmpty(MAX_EMITTERS * sizeof(EmitterShaderValues), GL_DYNAMIC_STORAGE_BIT);

    emitShader_ = new gl::ShaderPipeline({
        new gl::ShaderProgram("assets/shaders/particles/particles_emitter.comp"),
    });
    emitShader_->setDebugLabel("particle_system/emit_shader");

    updateShader_ = new gl::ShaderPipeline({
        new gl::ShaderProgram("assets/shaders/particles/particles.comp"),
    });
    updateShader_->setDebugLabel("particle_system/update_shader");

    drawShader_ = new gl::ShaderPipeline({
        new gl::ShaderProgram("assets/shaders/particles/particles.vert"),
        new gl::ShaderProgram("assets/shaders/particles/particles.frag"),
    });
    drawShader_->setDebugLabel("particle_system/draw_shader");

    resetShader_ = new gl::ShaderPipeline({
        new gl::ShaderProgram("assets/shaders/particles/particles_reset.comp"),
    });
    resetShader_->setDebugLabel("particle_system/reset_shader");

    gl::Buffer *quad_vbo = new gl::Buffer();
    quad_vbo->setDebugLabel("particle_system/quad_vbo");
    glm::vec2 quad_verts[] = {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}};
    quad_vbo->allocate(&quad_verts, sizeof(quad_verts), 0);

    quad_ = new gl::VertexArray();
    quad_->setDebugLabel("particle_system/quad_vao");
    quad_->layout(0, 0, 2, GL_FLOAT, false, 0);
    quad_->bindBuffer(0, *quad_vbo, 0, 2 * 4);
    quad_->own(quad_vbo);

    spriteSampler_ = new gl::Sampler();
    spriteSampler_->setDebugLabel("particle_system/sprite_sampler");
    spriteSampler_->filterMode(GL_NEAREST, GL_LINEAR);
    spriteSampler_->wrapMode(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER, 0);
    spriteSampler_->borderColor(glm::vec4{0.0, 0.0, 0.0, 0.0});

    tableSampler_ = new gl::Sampler();
    tableSampler_->setDebugLabel("particle_system/table_sampler");
    tableSampler_->filterMode(GL_LINEAR, GL_LINEAR);
    tableSampler_->wrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0);

    segmentation_.push_back(std::make_pair(0, capacity_));
    for (int i = MAX_EMITTERS - 1; i >= 0; i--) {
        freeEmitterIndices_.push_back(i);
    }
}

ParticleSystem::~ParticleSystem() {
    delete particleBuffer_;
    delete freeBuffer_;
    delete freeHeadsBuffer_;
    delete emitterBuffer_;
    delete emitShader_;
    delete updateShader_;
    delete drawShader_;
    delete resetShader_;
    delete quad_;
    delete spriteSampler_;
    delete tableSampler_;

    for (auto &&entry : materials_) {
        entry.second.destroy();
    }
}

struct Emission {
    glm::vec4 direction;
    glm::vec2 spread;
    glm::vec2 velocity;
    glm::vec2 life;
    glm::vec3 position;
    glm::vec4 rotation_revolutions;
    glm::vec2 size;
    glm::vec2 scale;
    glm::vec2 drag;
    glm::vec2 gravity;
    int index;
    int count;
};

void ParticleEmitter::update(float time_delta) {
    count_ = 0;
    timer_ += time_delta;
    while (timer_ >= interval_) {
        timer_ -= interval_;
        float frequency = 0.0;
        if (settings_.frequency.min != settings_.frequency.max) {
            frequency = settings_.frequency.range() * static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        }
        frequency += settings_.frequency.min;
        interval_ = 1.0f / frequency;
        if (settings_.count.min != settings_.count.max) {
            count_ += rand() % settings_.count.range();
        }
        count_ += settings_.count.min;
    }
}

ParticleEmitter::~ParticleEmitter() = default;

// FIXME: This is very slow to call for each individually
void ParticleSystem::emit_(Emission &emission) {
    auto comp = emitShader_->get(GL_COMPUTE_SHADER);
    comp->setUniform("u_random_seed", (unsigned int)rand());
    comp->setUniform("u_emission.direction", emission.direction);
    comp->setUniform("u_emission.spread", emission.spread);
    comp->setUniform("u_emission.velocity", emission.velocity);
    comp->setUniform("u_emission.life", emission.life);
    comp->setUniform("u_emission.position", emission.position);
    comp->setUniform("u_emission.rotation_revolutions", emission.rotation_revolutions);
    comp->setUniform("u_emission.size", emission.size);
    comp->setUniform("u_emission.scale", emission.scale);
    comp->setUniform("u_emission.drag", emission.drag);
    comp->setUniform("u_emission.gravity", emission.gravity);
    comp->setUniform("u_emission.index", emission.index);
    comp->setUniform("u_emission.count", emission.count);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleBuffer_->id());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, freeBuffer_->id());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, freeHeadsBuffer_->id());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, emitterBuffer_->id());
    glDispatchCompute(DIV_CEIL(emission.count, 64), 1, 1);
}

std::pair<int, int> ParticleSystem::allocateSegment_(int length) {
    std::pair<int, int> *free_segment = nullptr;
    for (int i = 0; i < segmentation_.size(); i++) {
        int free = segmentation_[i].second - segmentation_[i].first;
        if (length <= free) {
            free_segment = &segmentation_[i];
            break;
        }
    }
    if (free_segment == nullptr) {
        return std::make_pair(0, 0);
    }
    std::pair<int, int> result = std::make_pair(free_segment->first, free_segment->first + length);
    free_segment->first += length;
    reserved_ += length;
    return result;
}

void ParticleSystem::freeSegment_(int index, int length) {
    int start = index;
    int end = index + length;
    for (int i = 0; i < segmentation_.size(); i++) {
        auto &segment = segmentation_[i];
        // extend end of previous
        if (start == segment.second) {
            segment.second = end;
            break;
        }
        // extend start of next
        if (end == segment.first) {
            segment.first = start;
            break;
        }
        // add new segment
        if (end < segment.first) {
            segmentation_.insert(segmentation_.begin() + i, std::make_pair(start, end));
            break;
        }
    }

    // join segments and remove empties
    std::vector<std::pair<int, int>> filtered;
    filtered.reserve(segmentation_.size());
    std::pair<int, int> *active = nullptr;
    for (int i = 0; i < segmentation_.size(); i++) {
        auto &curr = segmentation_[i];
        if (curr.first == curr.second) {
            // zero length segement
            continue;
        }
        if (active && active->second == curr.first) {
            // extend previous segement
            active->second = curr.second;
        } else {
            filtered.push_back(curr);
            active = &filtered.back();
        }
    }
    segmentation_ = filtered;
    reserved_ -= length;
}

ParticleEmitter *ParticleSystem::add(ParticleSettings settings, std::string material) {
    if (freeEmitterIndices_.empty()) {
        PANIC("Maximum count of particle emitters reached");
    }
    int index = freeEmitterIndices_.back();
    freeEmitterIndices_.pop_back();

    int required_length = settings.count.max * (int)ceil(settings.life.max * settings.frequency.max);
    ParticleEmitter::Segment segment(allocateSegment_(required_length));
    if (segment.length == 0) {
        LOG_WARN("Not enough free space for particle emitter");
        emitterPool_[index] = ParticleEmitter(settings, ParticleEmitter::Segment(capacity_, 0));
        emitters_.push_back(&emitterPool_[index]);
        return &emitterPool_[index];
    }

    emitterPool_[index] = ParticleEmitter(settings, segment);
    emitterPool_[index].material = material;
    emitters_.push_back(&emitterPool_[index]);

    EmitterShaderValues emitter_values = EmitterShaderValues{
        .index_length = glm::ivec4(segment.index, segment.length, 0, 0),
        .gravity = glm::vec4(settings.gravity, 0.0f),
    };
    emitterBuffer_->write(index * sizeof(EmitterShaderValues), &emitter_values, sizeof(emitter_values));

    std::vector<GLuint> free_stack;
    free_stack.reserve(required_length);
    for (GLuint i = 0; i < (GLuint)required_length; i++) {
        free_stack.push_back(required_length - i - 1);
    }
    freeBuffer_->write(segment.index * sizeof(GLuint), free_stack.data(), free_stack.size() * sizeof(GLuint));
    freeHeadsBuffer_->write(index * sizeof(required_length), &required_length, sizeof(required_length));
    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

    return &emitterPool_[index];
}

void ParticleSystem::remove(ParticleEmitter *emitter) {
    if (emitter == nullptr)
        PANIC("Emitter is nullptr");
    size_t index = emitter - &emitterPool_[0];
    if (index < 0 || index >= emitterPool_.size())
        PANIC("Emitter is not part of this system");

    // Free segment and bookkeeping
    ParticleEmitter::Segment segment = emitter->segment();
    freeSegment_(segment.index, segment.length);
    freeEmitterIndices_.push_back(index);
    emitters_.erase(std::remove(emitters_.begin(), emitters_.end(), emitter), emitters_.end());

    resetShader_->bind();
    resetShader_->get(GL_COMPUTE_SHADER)->setUniform("u_segment", glm::ivec2(segment.index, segment.length));
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleBuffer_->id());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, freeBuffer_->id());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, freeHeadsBuffer_->id());
    glDispatchCompute(DIV_CEIL(segment.length, 64), 1, 1);
    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
}

void ParticleSystem::loadMaterial(std::string name, ParticleMaterialParams params) {
    gl::Texture *sprite = loader::texture(params.sprite, {.mipmap = false, .srgb = true});
    sprite->setDebugLabel("particle/sprite_tex");

    auto tint_image = loader::image(params.tint);
    gl::Texture *tint = new gl::Texture(GL_TEXTURE_1D_ARRAY);
    tint->setDebugLabel("particle/tint_tex");
    tint->allocate(1, GL_RGBA8, tint_image.width, tint_image.height);
    tint->load(0, tint_image.width, tint_image.height, GL_RGBA, GL_UNSIGNED_BYTE, tint_image.data.get());

    auto scale_image = loader::image(params.scale);
    gl::Texture *scale = new gl::Texture(GL_TEXTURE_2D);
    scale->setDebugLabel("particle/scale_tex");
    scale->allocate(1, GL_RG8, scale_image.width, scale_image.height);
    scale->load(0, scale_image.width, scale_image.height, GL_RGBA, GL_UNSIGNED_BYTE, scale_image.data.get());

    if (materials_.count(name) != 0) {
        materials_.at(name).destroy();
    }

    materials_[name] = ParticleMaterial{
        .blending = params.blending,
        .sprite = sprite,
        .tint = tint,
        .scale = scale,
    };
}

void ParticleSystem::update(float time_delta) {
    gl::pushDebugGroup("ParticleSystem::update");
    emitShader_->bind();
    for (int i = 0; i < emitterPool_.size(); i++) {
        ParticleEmitter &emitter = emitterPool_[i];
        ParticleSettings &settings = emitter.settings();

        if (!emitter.enabled)
            continue;

        emitter.update(time_delta);
        int count = emitter.intervalCount();
        if (count == 0) continue;

        Emission emission = {
            .direction = glm::vec4(glm::normalize(settings.direction), 0.0),
            .spread = glm::vec2(glm::radians(settings.spread.min), glm::radians(settings.spread.max)),
            .velocity = glm::vec2(settings.velocity.min, settings.velocity.max),
            .life = glm::vec2(settings.life.min, settings.life.max),
            .position = settings.position,
            .rotation_revolutions = glm::vec4(settings.rotation.min, settings.rotation.max, settings.revolutions.min / 60.0f, settings.revolutions.max / 60.0f),
            .size = settings.size,
            .scale = glm::vec2(settings.scale.min, settings.scale.max),
            .drag = glm::vec2(settings.drag.min, settings.drag.max),
            .gravity = glm::vec2(settings.gravityFactor.min, settings.gravityFactor.max),
            .index = i,
            .count = count,
        };

        emit_(emission);
    }
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    updateShader_->bind();
    updateShader_->get(GL_COMPUTE_SHADER)->setUniform("u_time_delta", time_delta);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleBuffer_->id());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, freeBuffer_->id());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, freeHeadsBuffer_->id());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, emitterBuffer_->id());
    glDispatchCompute(DIV_CEIL(capacity_, 64), 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    gl::popDebugGroup();
}

void ParticleSystem::draw(Camera &camera) {
    gl::pushDebugGroup("ParticleSystem::draw");
    gl::manager->setEnabled({gl::Capability::DepthTest, gl::Capability::Blend});
    gl::manager->depthFunc(gl::DepthFunc::GreaterOrEqual);
    gl::manager->depthMask(true);
    gl::manager->blendEquation(gl::BlendEquation::FuncAdd);

    drawShader_->bind();
    quad_->bind();
    drawShader_->get(GL_VERTEX_SHADER)->setUniform("u_projection_mat", camera.projectionMatrix());
    drawShader_->get(GL_VERTEX_SHADER)->setUniform("u_view_mat", camera.viewMatrix());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleBuffer_->id());

    spriteSampler_->bind(0);
    tableSampler_->bind(1);
    tableSampler_->bind(2);

    for (auto &&emitter : emitterPool_) {
        if (!emitter.enabled)
            continue;
        if (materials_.count(emitter.material) == 0)
            continue;
        ParticleMaterial &material = materials_.at(emitter.material);
        switch (material.blending) {
            case ParticleBlending::Additive:
                gl::manager->blendFuncSeparate(gl::BlendFactor::One, gl::BlendFactor::One, gl::BlendFactor::One, gl::BlendFactor::Zero);
                break;
            case ParticleBlending::AlphaClip:
                gl::manager->blendFuncSeparate(gl::BlendFactor::One, gl::BlendFactor::Zero, gl::BlendFactor::One, gl::BlendFactor::Zero);
                break;
        }
        material.sprite->bind(0);
        material.tint->bind(1);
        material.scale->bind(2);
        drawShader_->get(GL_VERTEX_SHADER)->setUniform("u_base_index", emitter.segment().index);
        drawShader_->get(GL_VERTEX_SHADER)->setUniform("u_emissivity", emitter.settings().emissivity);
        drawShader_->get(GL_VERTEX_SHADER)->setUniform("u_stretching", emitter.settings().stretching);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, emitter.segment().length);
    }
    gl::popDebugGroup();
}