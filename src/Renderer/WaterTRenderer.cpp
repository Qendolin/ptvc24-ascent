#include "WaterTRenderer.h"

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

WaterTRenderer::WaterTRenderer() {
    shader = new gl::ShaderPipeline(
        {new gl::ShaderProgram("assets/shaders/water/water.vert"),
         new gl::ShaderProgram("assets/shaders/water/water.frag"),
         new gl::ShaderProgram("assets/shaders/water/water.tesc"),
         new gl::ShaderProgram("assets/shaders/water/water.tese")});
    shader->setDebugLabel("water_renderer/shader");

    waterSampler = new gl::Sampler();
    waterSampler->setDebugLabel("water_renderer/waterSampler");
    waterSampler->wrapMode(GL_MIRRORED_REPEAT, GL_MIRRORED_REPEAT, 0);
    waterSampler->filterMode(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);  // mipmap levels aren't selected automatically in the tese shader.

    shadowSampler = new gl::Sampler();
    shadowSampler->setDebugLabel("water_renderer/shadow_sampler");
    shadowSampler->filterMode(GL_LINEAR, GL_LINEAR);
    shadowSampler->wrapMode(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER);
    shadowSampler->borderColor(glm::vec4(0));
    shadowSampler->compareMode(GL_COMPARE_REF_TO_TEXTURE, GL_GEQUAL);
}

WaterTRenderer::~WaterTRenderer() {
    delete shader;
    delete waterSampler;
}

void WaterTRenderer::render(Camera &camera, loader::Water &water, CSM &csm, loader::Environment &env) {
    gl::pushDebugGroup("WaterTRenderer::render");
    auto settings = Game::get().debugSettings.rendering.terrain;

    if (settings.wireframe)
        gl::manager->polygonMode(GL_FRONT_AND_BACK, GL_LINE);

    water.meshVao().bind();
    gl::manager->setEnabled({gl::Capability::DepthTest, gl::Capability::CullFace});
    gl::manager->depthFunc(gl::DepthFunc::GreaterOrEqual);
    gl::manager->depthMask(true);
    gl::manager->cullBack();
    shader->bind();

    water.heightTexture().bind(0);
    waterSampler->bind(0);
    water.normalTexture().bind(1);
    waterSampler->bind(1);

    env.diffuse().bind(4);
    env.cubemapSampler().bind(4);
    env.specular().bind(5);
    env.cubemapSampler().bind(5);
    env.brdfLut().bind(6);
    env.lutSampler().bind(6);

    shadowSampler->bind(7);
    csm.depthTexture()->bind(7);

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

    shader->fragmentStage()->setUniform("u_view_mat", camera.viewMatrix());
    shader->fragmentStage()->setUniform("u_camera_pos", camera.position);

    for (size_t i = 0; i < CSM::CASCADE_COUNT; i++) {
        CSMShadowCaster &caster = *csm.cascade(i);
        shader->fragmentStage()->setUniformIndexed("u_shadow_splits", i, caster.splitDistance);
        shader->get(GL_TESS_EVALUATION_SHADER)->setUniformIndexed("u_shadow_view_mat", i, caster.viewMatrix());
        shader->get(GL_TESS_EVALUATION_SHADER)->setUniformIndexed("u_shadow_projection_mat", i, caster.projectionMatrix());
    }

    glPatchParameteri(GL_PATCH_VERTICES, 4);
    glDrawArrays(GL_PATCHES, 0, water.patchCount());

    if (settings.wireframe)
        gl::manager->polygonMode(GL_FRONT_AND_BACK, GL_FILL);
    gl::popDebugGroup();
}