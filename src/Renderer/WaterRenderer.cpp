#include "WaterRenderer.h"

#include <glm/glm.hpp>
#include <iostream>

#include "../Camera.h"
#include "../GL/Geometry.h"
#include "../GL/Shader.h"
#include "../GL/StateManager.h"
#include "../GL/Texture.h"
#include "../Game.h"
#include "../Input.h"
#include "../Loader/Environment.h"
#include "../Loader/Water.h"
#include "../Util/Log.h"
#include "ShadowRenderer.h"

WaterRenderer::WaterRenderer() {
    shader = new gl::ShaderPipeline(
        {new gl::ShaderProgram("assets/shaders/water/water.vert"),
         new gl::ShaderProgram("assets/shaders/water/water.frag"),
         new gl::ShaderProgram("assets/shaders/water/water.tesc"),
         new gl::ShaderProgram("assets/shaders/water/water.tese")});
    shader->setDebugLabel("water_renderer/shader");

    waterSampler = new gl::Sampler();
    waterSampler->setDebugLabel("water_renderer/water_sampler");
    waterSampler->wrapMode(GL_MIRRORED_REPEAT, GL_MIRRORED_REPEAT, 0);
    waterSampler->filterMode(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);  // mipmap levels aren't selected automatically in the tese shader.

    depthSampler = new gl::Sampler();
    depthSampler->setDebugLabel("water_renderer/depth_sampler");
    depthSampler->wrapMode(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER, 0);
    depthSampler->filterMode(GL_NEAREST, GL_NEAREST);
}

WaterRenderer::~WaterRenderer() {
    delete shader;
    delete waterSampler;
    delete depthSampler;
}

void WaterRenderer::render(Camera &camera, loader::Water &water, loader::Environment &env, OrthoLight &sun, gl::Texture *depth) {
    gl::pushDebugGroup("WaterRenderer::render");
    auto settings = Game::get().debugSettings.rendering.water;

    if (settings.wireframe)
        gl::manager->polygonMode(GL_FRONT_AND_BACK, GL_LINE);

    water.meshVao().bind();
    gl::manager->setEnabled({gl::Capability::DepthTest, gl::Capability::CullFace, gl::Capability::Blend});
    gl::manager->depthFunc(gl::DepthFunc::GreaterOrEqual);
    gl::manager->depthMask(false);
    gl::manager->cullBack();
    gl::manager->blendFunc(gl::BlendFactor::SrcAlpha, gl::BlendFactor::OneMinusSrcAlpha);
    gl::manager->blendEquation(gl::BlendEquation::FuncAdd);
    shader->bind();

    water.heightTexture().bind(0);
    waterSampler->bind(0);

    depth->bind(2);
    depthSampler->bind(2);

    env.diffuse().bind(4);
    env.cubemapSampler().bind(4);
    env.specular().bind(5);
    env.cubemapSampler().bind(5);
    env.brdfLut().bind(6);
    env.lutSampler().bind(6);

    shader->vertexStage()->setUniform("u_position", water.origin());

    glm::vec3 camera_pos = camera.position;
    if (settings.fixedLodOrigin) {
        camera_pos = glm::vec3(0.0);
    }
    shader->get(GL_TESS_CONTROL_SHADER)->setUniform("u_camera_pos", camera_pos);

    shader->get(GL_TESS_EVALUATION_SHADER)->setUniform("u_projection_mat", camera.projectionMatrix());
    shader->get(GL_TESS_EVALUATION_SHADER)->setUniform("u_view_mat", camera.viewMatrix());
    shader->get(GL_TESS_EVALUATION_SHADER)->setUniform("u_height_scale", water.heightScale());
    shader->get(GL_TESS_EVALUATION_SHADER)->setUniform("u_time", (float)Game::get().input->time());

    shader->fragmentStage()->setUniform("u_light_dir[0]", sun.direction());
    shader->fragmentStage()->setUniform("u_light_radiance[0]", sun.radiance());

    shader->fragmentStage()->setUniform("u_near_plane", camera.nearPlane());
    shader->fragmentStage()->setUniform("u_camera_pos", camera.position);

    glPatchParameteri(GL_PATCH_VERTICES, 4);
    glDrawArrays(GL_PATCHES, 0, water.patchCount());

    if (settings.wireframe)
        gl::manager->polygonMode(GL_FRONT_AND_BACK, GL_FILL);
    gl::popDebugGroup();
}