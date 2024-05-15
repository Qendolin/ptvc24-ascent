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

MaterialBatchRenderer::MaterialBatchRenderer(std::shared_ptr<loader::IblEnv> env_diffuse,
                                             std::shared_ptr<loader::IblEnv> env_specular,
                                             std::shared_ptr<loader::FloatImage> brdf_lut) {
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

    lutSampler = new gl::Sampler();
    lutSampler->setDebugLabel("pbr_renderer/lut_sampler");
    lutSampler->wrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    lutSampler->filterMode(GL_LINEAR, GL_LINEAR);

    cubemapSampler = new gl::Sampler();
    cubemapSampler->setDebugLabel("pbr_renderer/cubemap_sampler");
    cubemapSampler->wrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    cubemapSampler->filterMode(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

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

    iblDiffuse = new gl::Texture(GL_TEXTURE_CUBE_MAP);
    iblDiffuse->setDebugLabel("pbr_renderer/environment/diffuse");
    iblDiffuse->allocate(1, GL_RGB16F, env_diffuse->baseSize, env_diffuse->baseSize);
    iblDiffuse->load(0, env_diffuse->baseSize, env_diffuse->baseSize, 6, GL_RGB, GL_FLOAT, env_diffuse->all().data());

    iblSpecular = new gl::Texture(GL_TEXTURE_CUBE_MAP);
    iblSpecular->setDebugLabel("pbr_renderer/environment/diffuse");
    iblSpecular->allocate(env_specular->levels, GL_RGB16F, env_specular->baseSize, env_specular->baseSize);
    for (int lvl = 0; lvl < env_specular->levels; lvl++) {
        iblSpecular->load(lvl, env_specular->size(lvl), env_specular->size(lvl), 6, GL_RGB, GL_FLOAT, env_specular->level(lvl).data());
    }

    iblBrdfLut = new gl::Texture(GL_TEXTURE_2D);
    iblBrdfLut->setDebugLabel("pbr_renderer/environment/brdf_lut");
    iblBrdfLut->allocate(1, GL_RG32F, brdf_lut->width, brdf_lut->height);
    iblBrdfLut->load(0, brdf_lut->width, brdf_lut->height, GL_RG, GL_FLOAT, brdf_lut->data.data());
}

MaterialBatchRenderer::~MaterialBatchRenderer() {
    delete shader;
    delete iblDiffuse;
    delete iblSpecular;
    delete iblBrdfLut;
    delete lutSampler;
    delete cubemapSampler;
    delete albedoSampler;
    delete normalSampler;
    delete ormSampler;
    delete shadowSampler;
}

void MaterialBatchRenderer::render(Camera &camera, loader::GraphicsData &graphics, ShadowCaster &shadow) {
    gl::pushDebugGroup("MaterialBatchRenderer::render");
    gl::manager->setEnabled({gl::Capability::DepthTest, gl::Capability::CullFace});
    gl::manager->depthMask(true);
    gl::manager->cullBack();
    gl::manager->depthFunc(gl::DepthFunc::GreaterOrEqual);

    shader->bind();
    shader->vertexStage()->setUniform("u_view_projection_mat", camera.viewProjectionMatrix());
    shader->fragmentStage()->setUniform("u_camera_pos", camera.position);

    albedoSampler->bind(0);
    ormSampler->bind(1);
    normalSampler->bind(2);
    cubemapSampler->bind(3);
    cubemapSampler->bind(4);
    lutSampler->bind(5);
    shadowSampler->bind(6);

    iblDiffuse->bind(3);
    iblSpecular->bind(4);
    iblBrdfLut->bind(5);

    shadow.depthTexture()->bind(6);
    shader->fragmentStage()->setUniform("u_shadow_bias", Game::get().debugSettings.rendering.shadow.depthBias);
    shader->vertexStage()->setUniform("u_shadow_view_mat", shadow.viewMatrix());
    shader->vertexStage()->setUniform("u_shadow_projection_mat", shadow.projectionMatrix());

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