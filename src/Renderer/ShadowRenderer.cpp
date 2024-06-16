#include "ShadowRenderer.h"

#include <glm/gtc/matrix_transform.hpp>

#include "../Camera.h"
#include "../Debug/Direct.h"
#include "../GL/Framebuffer.h"
#include "../GL/Geometry.h"
#include "../GL/Shader.h"
#include "../GL/StateManager.h"
#include "../GL/Texture.h"
#include "../Game.h"
#include "../Loader/Gltf.h"
#include "../Loader/Terrain.h"

ShadowCaster::~ShadowCaster() {
    delete depthTexture_;
    delete shadowMap_;
}

gl::Texture* ShadowCaster::depthTexture() const {
    return depthTexture_;
}

void ShadowCaster::bind() {
    shadowMap_->bind(GL_DRAW_FRAMEBUFFER);
}

void ShadowCaster::lookAt(glm::vec3 target, glm::vec3 direction, float distance, glm::vec3 up) {
    float dot = glm::dot(direction, up);
    if (dot < -0.99 || dot > 0.99) {
        // direction is too close to up vector, pick another one
        glm::vec3 abs = glm::abs(up);
        if (abs.x < abs.y && abs.x < abs.z)
            up = {1, 0, 0};
        else if (abs.y < abs.z)
            up = {0, 1, 0};
        else
            up = {0, 0, 1};
    }

    viewMatrix_ = glm::lookAt(target - glm::normalize(direction) * distance, target, up);
}

void ShadowCaster::lookAt(glm::vec3 target, float azimuth, float elevation, float distance, glm::vec3 up) {
    glm::vec3 direction = glm::vec3{
        glm::sin(azimuth) * glm::cos(elevation),
        glm::sin(elevation),
        glm::cos(azimuth) * glm::cos(elevation),
    };
    lookAt(target, -direction, distance, up);
}

OrthoShadowCaster::OrthoShadowCaster(int resolution, float extends, float near_plane, float far_plane) {
    resolution_ = resolution;

    shadowMap_ = new gl::Framebuffer();
    shadowMap_->setDebugLabel("ortho_shadow/fbo");

    depthTexture_ = new gl::Texture(GL_TEXTURE_2D);
    depthTexture_->setDebugLabel("ortho_shadow/depth_texture");
    // use a normalized integer format get an even value distribution
    depthTexture_->allocate(1, GL_DEPTH_COMPONENT32, resolution, resolution);
    shadowMap_->attachTexture(GL_DEPTH_ATTACHMENT, depthTexture_);
    shadowMap_->check(GL_DRAW_FRAMEBUFFER);

    // Reversed orthographic projection for 0-1 depth ragne.
    // Note: This doesn't bring any quality increase since the orthographic depth values are linear,
    // they don't use the perspective 'w' divide. I'm just using this to keep the depth ranges consistent.
    projectionMatrix_ = glm::mat4(
        2.0f / extends, 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f / extends, 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f / (near_plane - far_plane), 0.0f,
        0.0f, 0.0f, -far_plane / (near_plane - far_plane), 1.0f);
}

void OrthoShadowCaster::debugDraw() {
    DirectBuffer& dd = *Game::get().directDraw;
    dd.unshaded();
    dd.stroke(1.0f);
    dd.axes(glm::inverse(viewMatrix_), 2.0f);
    dd.push();
    dd.color(0.0f, 0.0f, 0.0f);
    dd.transform(glm::inverse(projectionMatrix_ * viewMatrix_));
    dd.boxLine(glm::vec3{0, 0, 0.5}, glm::vec3{2, 2, 1});
    dd.pop();
}

CSMShadowCaster::CSMShadowCaster() {
    shadowMap_ = nullptr;
}

void CSMShadowCaster::debugDraw() {
    DirectBuffer& dd = *Game::get().directDraw;
    dd.unshaded();
    dd.stroke(5.0f);
    dd.axes(glm::inverse(viewMatrix_), 2.0f);
    dd.push();
    dd.color(0.0f, 0.0f, 0.0f);
    dd.transform(glm::inverse(projectionMatrix_ * viewMatrix_));
    dd.boxLine(glm::vec3{0, 0, 0.5}, glm::vec3{2, 2, 1});
    dd.pop();
}

void CSMShadowCaster::bind() {
    shadowMap_->attachTexture(GL_DEPTH_ATTACHMENT, depthTexture_);
}

CSM::CSM(int resolution, float update_interval) : updateInterval_(update_interval) {
    shadowMap_ = new gl::Framebuffer();
    shadowMap_->setDebugLabel("csm/fbo");
    shadowMap_->bindTargets({});

    depthTexture_ = new gl::Texture(GL_TEXTURE_2D_ARRAY);
    depthTexture_->setDebugLabel("cms/depth_texture");
    // use a normalized integer format get an even value distribution
    depthTexture_->allocate(1, GL_DEPTH_COMPONENT16, resolution, resolution, CASCADE_COUNT);
    shadowMap_->attachTexture(GL_DEPTH_ATTACHMENT, depthTexture_);
    shadowMap_->check(GL_DRAW_FRAMEBUFFER);
    for (int i = 0; i < CASCADE_COUNT; i++) {
        auto view = depthTexture_->createView(GL_TEXTURE_2D, GL_DEPTH_COMPONENT16, 0, 0, i, i);
        cascades_[i].setDepthTexture(view, resolution);
        cascades_[i].setFramebuffer(shadowMap_);
    }
}

CSM::~CSM() {
    for (auto&& caster : cascades_) {
        caster.setFramebuffer(nullptr);
    }

    delete shadowMap_;
    delete depthTexture_;
}

bool CSM::update(Camera& camera, glm::vec3 light_dir, float time_delta) {
    lastUpdateElapsed_ += time_delta;
    if (lastUpdateElapsed_ < updateInterval_) {
        return false;
    }
    lastUpdateElapsed_ = 0.0f;

    float near_clip = 1.0;
    float far_clip = 1000.0;
    float clip_range = far_clip - near_clip;
    float min_z = near_clip;
    float max_z = near_clip + clip_range;

    glm::mat4 camera_projection_matrix = glm::perspective(camera.fov(), camera.aspect(), near_clip, far_clip);

    float range = max_z - min_z;
    float ratio = max_z / min_z;

    float cascade_splits[CASCADE_COUNT];

    auto settings = Game::get().debugSettings.rendering.shadow;

    // From https://developer.nvidia.com/gpugems/gpugems3/part-ii-light-and-shadows/chapter-10-parallel-split-shadow-maps-programmable-gpus
    for (int i = 0; i < CASCADE_COUNT; i++) {
        float p = (i + 1) / (float)(CASCADE_COUNT);
        float log = (float)(min_z * std::pow(ratio, p));
        float uniform = min_z + range * p;
        float d = settings.cascadeSplitLambda * (log - uniform) + uniform;
        cascade_splits[i] = (d - near_clip) / clip_range;
    }

    float lastSplitDist = 0.0f;
    for (int i = 0; i < CASCADE_COUNT; i++) {
        float splitDist = cascade_splits[i];

        glm::vec3 frustum_corners[] = {
            glm::vec3(-1.0f, 1.0f, -1.0f),
            glm::vec3(1.0f, 1.0f, -1.0f),
            glm::vec3(1.0f, -1.0f, -1.0f),
            glm::vec3(-1.0f, -1.0f, -1.0f),
            glm::vec3(-1.0f, 1.0f, 1.0f),
            glm::vec3(1.0f, 1.0f, 1.0f),
            glm::vec3(1.0f, -1.0f, 1.0f),
            glm::vec3(-1.0f, -1.0f, 1.0f),
        };

        // Project frustum corners into world space
        glm::mat4 inverse_camera = glm::inverse(camera_projection_matrix * camera.viewMatrix());
        for (int j = 0; j < 8; j++) {
            glm::vec4 invCorner = inverse_camera * glm::vec4(frustum_corners[j], 1.0f);
            frustum_corners[j] = glm::vec3(invCorner.x / invCorner.w, invCorner.y / invCorner.w, invCorner.z / invCorner.w);
        }

        // set distance
        for (int j = 0; j < 4; j++) {
            glm::vec3 dist = frustum_corners[j + 4] - frustum_corners[j];
            frustum_corners[j + 4] = frustum_corners[j] + dist * splitDist;
            frustum_corners[j] = frustum_corners[j] + dist * lastSplitDist;
        }

        glm::vec3 frustum_center(0.0);
        for (int j = 0; j < 8; j++) {
            frustum_center += frustum_corners[j];
        }
        frustum_center /= 8.0f;

        float radius = 0.0f;
        for (int j = 0; j < 8; j++) {
            float distance = glm::distance(frustum_corners[j], frustum_center);
            radius = std::max(radius, distance);
        }
        radius = (float)std::ceil(radius * 16.0f) / 16.0f;

        glm::vec3 max_extents = glm::vec3(radius);
        glm::vec3 min_extents = -max_extents;

        glm::vec3 eye = frustum_center - (light_dir * -min_extents.z);
        glm::vec3 up = glm::vec3(0.0f, 0.0f, 1.0f);
        glm::mat4 light_view_matrix = glm::lookAt(eye, frustum_center, up);
        glm::mat4 light_ortho_matrix = glm::orthoRH_ZO(min_extents.x, max_extents.x, min_extents.y, max_extents.y, 0.0f, max_extents.z - min_extents.z);

        // Reversed orthographic projection for 0-1 depth ragne.
        // Note: This doesn't bring any quality increase since the orthographic depth values are linear,
        // they don't use the perspective 'w' divide. I'm just using this to keep the depth ranges consistent.
        light_ortho_matrix = glm::mat4(
            2.0f / (2.0f * radius), 0.0f, 0.0f, 0.0f,
            0.0f, 2.0f / (2.0f * radius), 0.0f, 0.0f,
            0.0f, 0.0f, -1.0f / (0.0f - 2.0f * radius), 0.0f,
            0.0f, 0.0f, -2.0f * radius / (0.0f - 2.0f * radius), 1.0f);

        // Store split distance and matrix in cascade
        cascades_[i].lookAt(frustum_center, -light_dir, radius);
        cascades_[i].setProjectionMatrix(light_ortho_matrix);
        cascades_[i].splitDistance = (near_clip + splitDist * clip_range) * -1.0f;  // -1 because view z is negative

        lastSplitDist = cascade_splits[i];
    }
    return true;
}

void CSM::bind() {
    shadowMap_->bind(GL_DRAW_FRAMEBUFFER);
}

ShadowMapRenderer::ShadowMapRenderer() {
    objectShader = new gl::ShaderPipeline({
        new gl::ShaderProgram("assets/shaders/objects/shadow.vert"),
        new gl::ShaderProgram("assets/shaders/empty.frag"),
    });
    objectShader->setDebugLabel("shadow_map_renderer/object_shader");
    terrainShader = new gl::ShaderPipeline(
        {new gl::ShaderProgram("assets/shaders/terrain/terrain.vert"),
         new gl::ShaderProgram("assets/shaders/empty.frag"),
         new gl::ShaderProgram("assets/shaders/terrain/terrain.tesc"),
         new gl::ShaderProgram("assets/shaders/terrain/terrain_shadow.tese")});
    terrainShader->setDebugLabel("shadow_map_renderer/terrain_shader");

    terrainSampler = new gl::Sampler();
    terrainSampler->setDebugLabel("shadow_map_renderer/terrain_sampler");
    terrainSampler->wrapMode(GL_MIRRORED_REPEAT, GL_MIRRORED_REPEAT, 0);
    terrainSampler->filterMode(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
}

ShadowMapRenderer::~ShadowMapRenderer() {
    delete objectShader;
    delete terrainShader;
    delete terrainSampler;
}

void ShadowMapRenderer::render(CSM& csm, Camera& camera, loader::GraphicsData& graphics, loader::Terrain& terrain) {
    gl::pushDebugGroup("ShadowMapRenderer::render");

    auto settings = Game::get().debugSettings.rendering.shadow;

    auto prev_vp = gl::manager->getViewport();

    gl::manager->setEnabled({gl::Capability::DepthTest, gl::Capability::DepthClamp, gl::Capability::PolygonOffsetFill, gl::Capability::CullFace});
    gl::manager->depthFunc(gl::DepthFunc::GreaterOrEqual);
    gl::manager->depthMask(true);
    // TODO: figure out if front face culling actually improves shadow quality
    gl::manager->cullBack();
    // negative beacause of reversed z, I think this is correct
    gl::manager->polygonOffsetClamp(-settings.offsetFactor, -settings.offsetUnits, -settings.offsetClamp);
    glColorMask(false, false, false, false);
    glPatchParameteri(GL_PATCH_VERTICES, 4);

    csm.bind();

    for (size_t i = 0; i < CSM::CASCADE_COUNT; i++) {
        gl::pushDebugGroup("cascade_" + std::to_string(i));
        CSMShadowCaster& caster = *csm.cascade(i);
        caster.bind();
        gl::manager->setViewport(0, 0, caster.resolution(), caster.resolution());
        glClear(GL_DEPTH_BUFFER_BIT);

        if (settings.debugDrawEnabled)
            caster.debugDraw();

        // draw objects
        {
            objectShader->bind();
            graphics.bind();
            objectShader->vertexStage()->setUniform("u_view_mat", caster.viewMatrix());
            objectShader->vertexStage()->setUniform("u_projection_mat", caster.projectionMatrix());
            objectShader->vertexStage()->setUniform("u_size_bias", settings.sizeBias / (float)caster.resolution());

            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, 0, graphics.commandCount(), 0);
        }

        // draw terrain
        {
            terrain.meshVao().bind();
            terrainShader->bind();

            terrain.heightTexture().bind(0);
            terrainSampler->bind(0);

            terrainShader->vertexStage()->setUniform("u_position", terrain.origin());

            terrainShader->get(GL_TESS_CONTROL_SHADER)->setUniform("u_camera_pos", camera.position);

            terrainShader->get(GL_TESS_EVALUATION_SHADER)->setUniform("u_view_projection_mat", caster.projectionMatrix() * caster.viewMatrix());
            terrainShader->get(GL_TESS_EVALUATION_SHADER)->setUniform("u_height_scale", terrain.heightScale());

            glDrawArrays(GL_PATCHES, 0, terrain.patchCount());
        }
        gl::popDebugGroup();
    }
    glColorMask(true, true, true, true);
    gl::manager->setViewport(prev_vp[0], prev_vp[1], prev_vp[2], prev_vp[3]);

    gl::popDebugGroup();
}
