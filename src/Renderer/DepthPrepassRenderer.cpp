#include "DepthPrepassRenderer.h"

#include "../Camera.h"
#include "../GL/Framebuffer.h"
#include "../GL/Geometry.h"
#include "../GL/Shader.h"
#include "../GL/StateManager.h"
#include "../GL/Texture.h"
#include "../Game.h"
#include "../Loader/Gltf.h"
#include "../Loader/Terrain.h"

DepthPrepassRenderer::DepthPrepassRenderer() {
    objectShader = new gl::ShaderPipeline({
        new gl::ShaderProgram("assets/shaders/objects/depth_prepass.vert"),
        new gl::ShaderProgram("assets/shaders/empty.frag"),
    });
    objectShader->setDebugLabel("depth_prepass_renderer/object_shader");
    terrainShader = new gl::ShaderPipeline(
        {new gl::ShaderProgram("assets/shaders/terrain/terrain.vert"),
         new gl::ShaderProgram("assets/shaders/empty.frag"),
         new gl::ShaderProgram("assets/shaders/terrain/terrain.tesc"),
         new gl::ShaderProgram("assets/shaders/terrain/terrain_depth_prepass.tese")});
    terrainShader->setDebugLabel("depth_prepass_renderer/terrain_shader");

    terrainSampler = new gl::Sampler();
    terrainSampler->setDebugLabel("depth_prepass_renderer/terrain_sampler");
    terrainSampler->wrapMode(GL_MIRRORED_REPEAT, GL_MIRRORED_REPEAT, 0);
    terrainSampler->filterMode(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
}

DepthPrepassRenderer::~DepthPrepassRenderer() {
    delete objectShader;
    delete terrainShader;
    delete terrainSampler;
}

void DepthPrepassRenderer::render(Camera& camera, loader::GraphicsData& graphics, loader::Terrain& terrain) {
    gl::pushDebugGroup("DepthPrepassRenderer::render");

    gl::manager->setEnabled({gl::Capability::DepthTest, gl::Capability::CullFace});
    gl::manager->depthFunc(gl::DepthFunc::GreaterOrEqual);
    gl::manager->depthMask(true);
    gl::manager->cullBack();
    glPatchParameteri(GL_PATCH_VERTICES, 4);
    glColorMask(false, false, false, false);

    // draw objects
    {
        objectShader->bind();
        graphics.bind();
        objectShader->vertexStage()->setUniform("u_view_mat", camera.viewMatrix());
        objectShader->vertexStage()->setUniform("u_projection_mat", camera.projectionMatrix());

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

        terrainShader->get(GL_TESS_EVALUATION_SHADER)->setUniform("u_view_mat", camera.viewMatrix());
        terrainShader->get(GL_TESS_EVALUATION_SHADER)->setUniform("u_projection_mat", camera.projectionMatrix());
        terrainShader->get(GL_TESS_EVALUATION_SHADER)->setUniform("u_height_scale", terrain.heightScale());

        glDrawArrays(GL_PATCHES, 0, terrain.patchCount());
    }

    glColorMask(true, true, true, true);

    gl::popDebugGroup();
}