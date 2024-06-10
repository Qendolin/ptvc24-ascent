#include "TerrainRenderer.h"

#include <glm/glm.hpp>
#include <iostream>

#include "../Camera.h"
#include "../GL/Geometry.h"
#include "../GL/Shader.h"
#include "../GL/StateManager.h"
#include "../GL/Texture.h"
#include "../Game.h"
#include "../Loader/Environment.h"
#include "../Loader/Terrain.h"
#include "../Scene/Light.h"
#include "../Util/Log.h"
#include "ShadowRenderer.h"

TerrainRenderer::TerrainRenderer() {
    shader = new gl::ShaderPipeline(
        {new gl::ShaderProgram("assets/shaders/terrain/terrain.vert"),
         new gl::ShaderProgram("assets/shaders/terrain/terrain.frag"),
         new gl::ShaderProgram("assets/shaders/terrain/terrain.tesc"),
         new gl::ShaderProgram("assets/shaders/terrain/terrain.tese")});
    shader->setDebugLabel("terrain_renderer/shader");

    terrainSampler = new gl::Sampler();
    terrainSampler->setDebugLabel("terrain_renderer/terrainSampler");
    terrainSampler->wrapMode(GL_MIRRORED_REPEAT, GL_MIRRORED_REPEAT, 0);
    terrainSampler->filterMode(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);  // mipmap levels aren't selected automatically in the tese shader.

    shadowSampler = new gl::Sampler();
    shadowSampler->setDebugLabel("terrain_renderer/shadow_sampler");
    shadowSampler->filterMode(GL_LINEAR, GL_LINEAR);
    shadowSampler->wrapMode(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER);
    shadowSampler->borderColor(glm::vec4(0));
    shadowSampler->compareMode(GL_COMPARE_REF_TO_TEXTURE, GL_GEQUAL);
}

TerrainRenderer::~TerrainRenderer() {
    delete shader;
    delete terrainSampler;
    delete shadowSampler;
}

void TerrainRenderer::render(Camera &camera, loader::Terrain &terrain, CSM &csm, loader::Environment &env, OrthoLight &sun) {
    gl::pushDebugGroup("TerrainRenderer::render");
    auto settings = Game::get().debugSettings.rendering.terrain;

    if (settings.wireframe)
        gl::manager->polygonMode(GL_FRONT_AND_BACK, GL_LINE);

    terrain.meshVao().bind();
    gl::manager->setEnabled({gl::Capability::DepthTest, gl::Capability::CullFace});
    gl::manager->depthFunc(gl::DepthFunc::GreaterOrEqual);
    gl::manager->depthMask(true);
    gl::manager->cullBack();
    shader->bind();

    terrain.heightTexture().bind(0);
    terrainSampler->bind(0);
    terrain.albedoTexture().bind(1);
    terrainSampler->bind(1);
    terrain.normalTexture().bind(2);
    terrainSampler->bind(2);
    terrain.occlusionTexture().bind(3);
    terrainSampler->bind(3);

    env.diffuse().bind(4);
    env.cubemapSampler().bind(4);
    env.specular().bind(5);
    env.cubemapSampler().bind(5);
    env.brdfLut().bind(6);
    env.lutSampler().bind(6);

    shadowSampler->bind(7);
    csm.depthTexture()->bind(7);

    shader->vertexStage()->setUniform("u_position", terrain.origin());

    glm::vec3 camera_pos = camera.position;
    if (settings.fixedLodOrigin) {
        camera_pos = glm::vec3(0.0);
    }
    shader->get(GL_TESS_CONTROL_SHADER)->setUniform("u_camera_pos", camera_pos);

    shader->get(GL_TESS_EVALUATION_SHADER)->setUniform("u_projection_mat", camera.projectionMatrix());
    shader->get(GL_TESS_EVALUATION_SHADER)->setUniform("u_view_mat", camera.viewMatrix());
    shader->get(GL_TESS_EVALUATION_SHADER)->setUniform("u_height_scale", terrain.heightScale());

    shader->fragmentStage()->setUniform("u_view_mat", camera.viewMatrix());
    shader->fragmentStage()->setUniform("u_camera_pos", camera.position);
    shader->fragmentStage()->setUniform("u_light_dir[0]", sun.direction());
    shader->fragmentStage()->setUniform("u_light_radiance[0]", sun.radiance());

    for (size_t i = 0; i < CSM::CASCADE_COUNT; i++) {
        CSMShadowCaster &caster = *csm.cascade(i);
        shader->fragmentStage()->setUniformIndexed("u_shadow_splits", i, caster.splitDistance);
        shader->get(GL_TESS_EVALUATION_SHADER)->setUniformIndexed("u_shadow_view_mat", i, caster.viewMatrix());
        shader->get(GL_TESS_EVALUATION_SHADER)->setUniformIndexed("u_shadow_projection_mat", i, caster.projectionMatrix());
    }

    glPatchParameteri(GL_PATCH_VERTICES, 4);
    glDrawArrays(GL_PATCHES, 0, terrain.patchCount());

    if (settings.wireframe)
        gl::manager->polygonMode(GL_FRONT_AND_BACK, GL_FILL);
    gl::popDebugGroup();
}