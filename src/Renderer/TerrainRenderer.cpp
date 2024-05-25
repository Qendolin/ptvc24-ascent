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
#include "../Util/Log.h"

TerrainRenderer::TerrainRenderer() {
    shader = new gl::ShaderPipeline(
        {new gl::ShaderProgram("assets/shaders/terrain.vert"),
         new gl::ShaderProgram("assets/shaders/terrain.frag"),
         new gl::ShaderProgram("assets/shaders/terrain.tesc"),
         new gl::ShaderProgram("assets/shaders/terrain.tese")});
    shader->setDebugLabel("terrain_renderer/shader");

    sampler = new gl::Sampler();
    sampler->setDebugLabel("terrain_renderer/sampler");
    sampler->wrapMode(GL_MIRRORED_REPEAT, GL_MIRRORED_REPEAT, 0);
    sampler->filterMode(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);  // mipmap levels aren't selected automatically in the tese shader.
}

TerrainRenderer::~TerrainRenderer() {
    delete shader;
    delete sampler;
}

void TerrainRenderer::render(Camera &camera, loader::Terrain &terrain, loader::Environment &env) {
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
    sampler->bind(0);
    terrain.albedoTexture().bind(1);
    sampler->bind(1);
    terrain.normalTexture().bind(2);
    sampler->bind(2);
    terrain.occlusionTexture().bind(3);
    sampler->bind(3);

    env.diffuse().bind(4);
    env.cubemapSampler().bind(4);
    env.specular().bind(5);
    env.cubemapSampler().bind(5);
    env.brdfLut().bind(6);
    env.lutSampler().bind(6);

    shader->vertexStage()->setUniform("u_position", terrain.position);

    glm::vec3 origin = camera.position;
    if (settings.fixedLodOrigin) {
        origin = glm::vec3(0.0);
    }

    shader->get(GL_TESS_CONTROL_SHADER)->setUniform("u_camera_pos", origin);

    shader->get(GL_TESS_EVALUATION_SHADER)->setUniform("u_view_projection_mat", camera.viewProjectionMatrix());
    shader->get(GL_TESS_EVALUATION_SHADER)->setUniform("u_height_scale", settings.heightScale);

    shader->fragmentStage()->setUniform("u_view_mat", camera.viewMatrix());
    shader->fragmentStage()->setUniform("u_camera_pos", camera.position);

    glPatchParameteri(GL_PATCH_VERTICES, 4);
    glDrawArrays(GL_PATCHES, 0, terrain.patchCount());

    if (settings.wireframe)
        gl::manager->polygonMode(GL_FRONT_AND_BACK, GL_FILL);
    gl::popDebugGroup();
}