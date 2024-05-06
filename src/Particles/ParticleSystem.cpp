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
    glm::ivec4 offset_capacity;
    glm::vec4 gravity;
};

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
        new gl::ShaderProgram("assets/shaders/particles_emitter.comp"),
    });
    emitShader_->setDebugLabel("particle_system/emit_shader");

    updateShader_ = new gl::ShaderPipeline({
        new gl::ShaderProgram("assets/shaders/particles.comp"),
    });
    updateShader_->setDebugLabel("particle_system/update_shader");

    drawShader_ = new gl::ShaderPipeline({
        new gl::ShaderProgram("assets/shaders/particles.vert"),
        new gl::ShaderProgram("assets/shaders/particles.frag"),
    });
    drawShader_->setDebugLabel("particle_system/draw_shader");

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
}

ParticleSystem::~ParticleSystem() {
    delete particleBuffer_;
    delete freeBuffer_;
    delete freeHeadsBuffer_;
    delete emitterBuffer_;
    delete emitShader_;
    delete updateShader_;
    delete drawShader_;
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
    glm::vec4 color;
    glm::vec4 hslVariation;
    glm::vec2 emissivity;
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

void ParticleSystem::emit_(Emission &emission) {
    auto comp = emitShader_->get(GL_COMPUTE_SHADER);
    comp->setUniform("u_random_seed", (unsigned int)rand());
    comp->setUniform("u_emission.direction", emission.direction);
    comp->setUniform("u_emission.spread", emission.spread);
    comp->setUniform("u_emission.velocity", emission.velocity);
    comp->setUniform("u_emission.life", emission.life);
    comp->setUniform("u_emission.position", emission.position);
    comp->setUniform("u_emission.rotation_revolutions", emission.rotation_revolutions);
    comp->setUniform("u_emission.color", emission.color);
    comp->setUniform("u_emission.hslVariation", emission.hslVariation);
    comp->setUniform("u_emission.emissivity", emission.emissivity);
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

std::pair<int, int> *ParticleSystem::allocateSegment_(int length) {
    std::pair<int, int> *free_fragment = nullptr;
    for (int i = 0; i < segmentation_.size(); i++) {
        int free = segmentation_[i].second - segmentation_[i].first;
        if (length <= free) {
            free_fragment = &segmentation_[i];
            break;
        }
    }
    if (free_fragment != nullptr) {
        free_fragment->first += length;
    }
    return free_fragment;
}

void ParticleSystem::freeSegment_(int index, int length) {
    int start = index;
    int end = index + length;
    for (int i = 0; i < segmentation_.size(); i++) {
        const auto &segment = segmentation_[i];
        // extend end of previous
        if (start == segment.second + 1) {
            segmentation_[i].second = start;
            break;
        }
        // extend start of next
        if (end == segment.first - 1) {
            segmentation_[i].first = start;
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
    for (int i = 1; i < segmentation_.size(); i++) {
        auto &curr = segmentation_[i];
        if (curr.first == curr.second) {
            continue;
        }
        if (active && active->second == curr.first) {
            active->second = curr.second;
        } else {
            filtered.push_back(curr);
            active = &filtered.back();
        }
    }
    segmentation_ = filtered;
}

ParticleEmitter *ParticleSystem::add(ParticleSettings settings, std::string material) {
    int index = emittersCount_++;
    if (index >= MAX_EMITTERS) {
        PANIC("Max particle emitter count exceeded");
    }

    int required_length = settings.count.max * (int)ceil(settings.life.max * settings.frequency.max);
    std::pair<int, int> *free_fragment = allocateSegment_(required_length);
    if (free_fragment == nullptr) {
        LOG_WARN("Not enough free space for particle emitter");
        emitters_[index] = ParticleEmitter(settings, ParticleEmitter::Segment{.index = capacity_, .length = 0});
        return &emitters_[index];
    }

    emitters_[index] = ParticleEmitter(settings, ParticleEmitter::Segment{.index = reserved_, .length = required_length});
    emitters_[index].material = material;

    EmitterShaderValues emitter_values = EmitterShaderValues{
        .offset_capacity = glm::ivec4(reserved_, required_length, 0, 0),
        .gravity = glm::vec4(settings.gravity, 0.0f),
    };
    emitterBuffer_->write(index * sizeof(EmitterShaderValues), &emitter_values, sizeof(emitter_values));

    std::vector<GLuint> free_stack;
    free_stack.reserve(required_length);
    for (GLuint i = 0; i < (GLuint)required_length; i++) {
        free_stack.push_back(required_length - i - 1);
    }
    freeBuffer_->write(reserved_ * sizeof(GLuint), free_stack.data(), free_stack.size() * sizeof(GLuint));
    freeHeadsBuffer_->write(index * sizeof(required_length), &required_length, sizeof(required_length));
    reserved_ += required_length;
    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

    return &emitters_[index];
}

void ParticleSystem::remove(ParticleEmitter *emitter) {
    if (emitter == nullptr)
        PANIC("Emitter is nullptr");
    size_t index = emitter - &emitters_[0];
    if (index < 0 || index >= emitters_.size())
        PANIC("Emitter is not part of this system");
    freeSegment_(emitter->segment().index, emitter->segment().length);
    std::shift_left(emitters_.begin() + index, emitters_.end(), 1);
    emittersCount_--;
}

void ParticleSystem::loadMaterial(std::string name, ParticleMaterialParams params) {
    gl::Texture *sprite = loader::texture(params.sprite, {.mipmap = false, .srgb = true});

    auto tint_image = loader::image(params.tint);
    gl::Texture *tint = new gl::Texture(GL_TEXTURE_1D_ARRAY);
    tint->allocate(1, GL_RGBA8, tint_image.width, tint_image.height);
    tint->load(0, tint_image.width, tint_image.height, GL_RGBA, GL_UNSIGNED_BYTE, tint_image.data.get());

    auto scale_image = loader::image(params.scale);
    gl::Texture *scale = new gl::Texture(GL_TEXTURE_2D);
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
    for (int i = 0; i < emitters_.size(); i++) {
        ParticleEmitter &emitter = emitters_[i];
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
            .color = glm::vec4(settings.color, 0.0),
            .hslVariation = glm::vec4(settings.hslVariation, 0.0),
            .emissivity = glm::vec2(settings.emissivity.min, settings.emissivity.max),
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
    glDispatchCompute(DIV_CEIL(reserved_, 64), 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    gl::popDebugGroup();
}

void ParticleSystem::draw(Camera &camera) {
    gl::pushDebugGroup("ParticleSystem::draw");
    gl::manager->setEnabled({gl::Capability::DepthTest, gl::Capability::Blend, gl::Capability::CullFace});
    gl::manager->depthFunc(gl::DepthFunc::GreaterOrEqual);
    gl::manager->depthMask(true);
    gl::manager->cullBack();
    gl::manager->blendEquation(gl::BlendEquation::FuncAdd);

    drawShader_->bind();
    quad_->bind();
    drawShader_->get(GL_VERTEX_SHADER)->setUniform("u_projection_mat", camera.projectionMatrix());
    drawShader_->get(GL_VERTEX_SHADER)->setUniform("u_view_mat", camera.viewMatrix());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particleBuffer_->id());

    spriteSampler_->bind(0);
    tableSampler_->bind(1);
    tableSampler_->bind(2);

    for (auto &&emitter : emitters_) {
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
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, emitter.segment().length);
    }
    gl::popDebugGroup();
}