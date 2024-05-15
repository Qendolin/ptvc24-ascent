#include "ShadowRenderer.h"

#include <glm/gtc/matrix_transform.hpp>

#include "../Debug/Direct.h"
#include "../GL/Framebuffer.h"
#include "../GL/Shader.h"
#include "../GL/StateManager.h"
#include "../GL/Texture.h"
#include "../Game.h"
#include "../Loader/Gltf.h"

ShadowCaster::~ShadowCaster() {
    delete shadowMap_->getTexture(GL_DEPTH_ATTACHMENT);
    delete shadowMap_;
}

gl::Texture* ShadowCaster::depthTexture() const {
    return shadowMap_->getTexture(GL_DEPTH_ATTACHMENT);
}

void ShadowCaster::bind() {
    shadowMap_->bind(GL_DRAW_FRAMEBUFFER);
}

void ShadowCaster::lookAt(glm::vec3 target, glm::vec3 direction, float distance, glm::vec3 up) {
    if (glm::dot(direction, up) < -0.99) {
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

    gl::Texture* depth_texture = new gl::Texture(GL_TEXTURE_2D);
    depth_texture->setDebugLabel("ortho_shadow/depth_texture");
    // use a normalized integer format get an even value distribution
    depth_texture->allocate(1, GL_DEPTH_COMPONENT32, resolution, resolution);
    shadowMap_->attachTexture(GL_DEPTH_ATTACHMENT, depth_texture);
    shadowMap_->check(GL_DRAW_FRAMEBUFFER);

    // Reversed orthographic projection for 0-1 depth ragne. credit: Wendelin (myself)
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
    dd.stroke(0.1f);
    dd.axes(glm::inverse(viewMatrix_), 2.0f);
    dd.push();
    dd.color(0.0f, 0.0f, 0.0f);
    dd.transform(glm::inverse(projectionMatrix_ * viewMatrix_));
    dd.boxLine(glm::vec3{0, 0, 0.5}, glm::vec3{2, 2, 1});
    dd.pop();
}

ShadowMapRenderer::ShadowMapRenderer() {
    shader = new gl::ShaderPipeline({
        new gl::ShaderProgram("assets/shaders/shadow.vert"),
        new gl::ShaderProgram("assets/shaders/empty.frag"),
    });
    shader->setDebugLabel("shadow_map_renderer/shader");
}

ShadowMapRenderer::~ShadowMapRenderer() {
    delete shader;
}

void ShadowMapRenderer::render(ShadowCaster& caster, loader::GraphicsData& graphics) {
    gl::pushDebugGroup("ShadowMapRenderer::render");

    caster.bind();
    shader->bind();
    graphics.bind();

    auto settings = Game::get().debugSettings.rendering.shadow;

    if (settings.debugDrawEnabled)
        caster.debugDraw();

    shader->vertexStage()->setUniform("u_view_mat", caster.viewMatrix());
    shader->vertexStage()->setUniform("u_projection_mat", caster.projectionMatrix());
    shader->vertexStage()->setUniform("u_size_bias", settings.normalBias);
    auto prev_vp = gl::manager->getViewport();
    gl::manager->setViewport(0, 0, caster.resolution(), caster.resolution());
    gl::manager->setEnabled({gl::Capability::DepthTest, gl::Capability::DepthClamp, gl::Capability::PolygonOffsetFill, gl::Capability::CullFace});
    gl::manager->depthFunc(gl::DepthFunc::GreaterOrEqual);
    gl::manager->depthMask(true);
    // TODO: figure out if front face culling actually improves shadow quality
    gl::manager->cullBack();
    // negative beacause of reversed z, I think this is correct
    gl::manager->polygonOffsetClamp(-settings.offsetFactor, -settings.offsetUnits, -settings.offsetClamp);

    glClear(GL_DEPTH_BUFFER_BIT);
    glColorMask(false, false, false, false);
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, 0, graphics.commandCount(), 0);
    glColorMask(true, true, true, true);
    gl::manager->setViewport(prev_vp[0], prev_vp[1], prev_vp[2], prev_vp[3]);

    gl::popDebugGroup();
}