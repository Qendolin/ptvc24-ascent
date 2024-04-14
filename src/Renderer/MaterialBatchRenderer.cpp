#include "MaterialBatchRenderer.h"

#include "../Camera.h"
#include "../GL/Geometry.h"
#include "../GL/Shader.h"
#include "../GL/StateManager.h"
#include "../GL/Texture.h"
#include "../Loader/Gltf.h"

MaterialBatchRenderer::MaterialBatchRenderer() {
    shader = new gl::ShaderPipeline(
        {new gl::ShaderProgram("assets/shaders/test.vert"),
         new gl::ShaderProgram("assets/shaders/test.frag")});
    shader->setDebugLabel("material_batch_renderer/shader");
}

MaterialBatchRenderer::~MaterialBatchRenderer() {
    delete shader;
}

void MaterialBatchRenderer::render(Camera &camera, loader::GraphicsData &graphics) {
    gl::pushDebugGroup("MaterialBatchRenderer::render");
    gl::manager->setEnabled({gl::Capability::DepthTest, gl::Capability::CullFace});
    gl::manager->depthMask(true);
    gl::manager->cullBack();
    gl::manager->depthFunc(gl::DepthFunc::GreaterOrEqual);

    shader->bind();
    shader->vertexStage()->setUniform("u_view_projection_mat", camera.viewProjectionMatrix());
    shader->fragmentStage()->setUniform("u_camera_pos", camera.position);

    graphics.bind();
    for (auto &&batch : graphics.batches) {
        auto &material = graphics.materials[batch.material];
        auto &defaultMaterial = graphics.defaultMaterial;
        shader->fragmentStage()->setUniform("u_albedo_fac", material.albedoFactor);
        shader->fragmentStage()->setUniform("u_metallic_roughness_fac", material.metallicRoughnessFactor);
        if (material.albedo == nullptr) {
            defaultMaterial.albedo->bind(0);
        } else {
            material.albedo->bind(0);
        }
        if (material.occlusionMetallicRoughness == nullptr) {
            defaultMaterial.occlusionMetallicRoughness->bind(1);
        } else {
            material.occlusionMetallicRoughness->bind(1);
        }
        if (material.normal == nullptr) {
            defaultMaterial.normal->bind(2);
        } else {
            material.normal->bind(2);
        }
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, batch.commandOffset, batch.commandCount, 0);
    }
    gl::popDebugGroup();
}