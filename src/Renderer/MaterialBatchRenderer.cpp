#include "MaterialBatchRenderer.h"

#include "../Camera.h"
#include "../GL/Geometry.h"
#include "../GL/Shader.h"
#include "../GL/StateManager.h"
#include "../GL/Texture.h"
#include "../Game.h"
#include "../Loader/Environment.h"
#include "../Loader/Gltf.h"
#include "ShadowRenderer.h"

MaterialBatchRenderer::MaterialBatchRenderer() {
    shader = new gl::ShaderPipeline(
        {new gl::ShaderProgram("assets/shaders/pbr.vert"),
         new gl::ShaderProgram("assets/shaders/pbr.frag")});
    shader->setDebugLabel("pbr_renderer/shader");

    shadowSampler = new gl::Sampler();
    shadowSampler->setDebugLabel("pbr_renderer/shadow_sampler");
    shadowSampler->filterMode(GL_LINEAR, GL_LINEAR);
    shadowSampler->wrapMode(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER);
    shadowSampler->borderColor(glm::vec4(0));
    shadowSampler->compareMode(GL_COMPARE_REF_TO_TEXTURE, GL_GEQUAL);

    albedoSampler = new gl::Sampler();
    albedoSampler->setDebugLabel("pbr_renderer/albedo_sampler");
    albedoSampler->filterMode(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
    albedoSampler->wrapMode(GL_REPEAT, GL_REPEAT, 0);
    albedoSampler->anisotropicFilter(gl::manager->clampAnisotropy(8.0));

    normalSampler = new gl::Sampler();
    normalSampler->setDebugLabel("pbr_renderer/normal_sampler");
    normalSampler->filterMode(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
    normalSampler->wrapMode(GL_REPEAT, GL_REPEAT, 0);
    normalSampler->anisotropicFilter(gl::manager->clampAnisotropy(8.0));

    ormSampler = new gl::Sampler();
    ormSampler->setDebugLabel("pbr_renderer/orm_sampler");
    ormSampler->filterMode(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
    ormSampler->wrapMode(GL_REPEAT, GL_REPEAT, 0);
    ormSampler->anisotropicFilter(gl::manager->clampAnisotropy(8.0));
    ormSampler->lodBias(-2.0);
}

MaterialBatchRenderer::~MaterialBatchRenderer() {
    delete shader;
    delete albedoSampler;
    delete normalSampler;
    delete ormSampler;
    delete shadowSampler;
}

void MaterialBatchRenderer::render(Camera &camera, loader::GraphicsData &graphics, CSM &csm, loader::Environment &env) {
    gl::pushDebugGroup("MaterialBatchRenderer::render");
    gl::manager->setEnabled({gl::Capability::DepthTest, gl::Capability::CullFace});
    gl::manager->depthMask(true);
    gl::manager->cullBack();
    gl::manager->depthFunc(gl::DepthFunc::GreaterOrEqual);

    shader->bind();
    shader->vertexStage()->setUniform("u_projection_mat", camera.projectionMatrix());
    shader->vertexStage()->setUniform("u_view_mat", camera.viewMatrix());
    shader->fragmentStage()->setUniform("u_camera_pos", camera.position);

    albedoSampler->bind(0);
    ormSampler->bind(1);
    normalSampler->bind(2);
    env.cubemapSampler().bind(3);
    env.cubemapSampler().bind(4);
    env.lutSampler().bind(5);

    env.diffuse().bind(3);
    env.specular().bind(4);
    env.brdfLut().bind(5);

    shadowSampler->bind(6);
    csm.depthTexture()->bind(6);

    shader->fragmentStage()->setUniform("u_shadow_depth_bias", Game::get().debugSettings.rendering.shadow.depthBias);
    shader->fragmentStage()->setUniform("u_view_mat", camera.viewMatrix());
    shader->vertexStage()->setUniform("u_shadow_normal_bias", Game::get().debugSettings.rendering.shadow.normalBias);
    for (size_t i = 0; i < CSM::CASCADE_COUNT; i++) {
        CSMShadowCaster &caster = *csm.cascade(i);
        shader->fragmentStage()->setUniformIndexed("u_shadow_splits", i, caster.splitDistance);
        shader->vertexStage()->setUniformIndexed("u_shadow_view_mat", i, caster.viewMatrix());
        shader->vertexStage()->setUniformIndexed("u_shadow_projection_mat", i, caster.projectionMatrix());
    }

    graphics.bind();
    for (auto &&batch : graphics.batches) {
        auto &defaultMaterial = graphics.defaultMaterial;
        auto &material = batch.material < 0 ? defaultMaterial : graphics.materials[batch.material];
        shader->fragmentStage()->setUniform("u_albedo_fac", material.albedoFactor);
        shader->fragmentStage()->setUniform("u_occlusion_metallic_roughness_fac", glm::vec3(1.0, material.metallicRoughnessFactor));
        if (Game::get().debugSettings.rendering.normalMapsEnabled) {
            shader->fragmentStage()->setUniform("u_normal_fac", material.normalFactor);
        } else {
            shader->fragmentStage()->setUniform("u_normal_fac", 0.0f);
        }

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