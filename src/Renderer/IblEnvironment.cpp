#include "IblEnvironment.h"

#include "../GL/Texture.h"

IblEnvironment::IblEnvironment(std::shared_ptr<loader::IblEnv> env_diffuse,
                               std::shared_ptr<loader::IblEnv> env_specular,
                               std::shared_ptr<loader::FloatImage> brdf_lut) {
    lutSampler_ = new gl::Sampler();
    lutSampler_->setDebugLabel("ibl_env/lut_sampler");
    lutSampler_->wrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    lutSampler_->filterMode(GL_LINEAR, GL_LINEAR);

    cubemapSampler_ = new gl::Sampler();
    cubemapSampler_->setDebugLabel("ibl_env/cubemap_sampler");
    cubemapSampler_->wrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    cubemapSampler_->filterMode(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

    diffuse_ = new gl::Texture(GL_TEXTURE_CUBE_MAP);
    diffuse_->setDebugLabel("ibl_env/environment/diffuse");
    diffuse_->allocate(1, GL_RGB16F, env_diffuse->baseSize, env_diffuse->baseSize);
    diffuse_->load(0, env_diffuse->baseSize, env_diffuse->baseSize, 6, GL_RGB, GL_FLOAT, env_diffuse->all().data());

    specular_ = new gl::Texture(GL_TEXTURE_CUBE_MAP);
    specular_->setDebugLabel("ibl_env/environment/diffuse");
    specular_->allocate(env_specular->levels, GL_RGB16F, env_specular->baseSize, env_specular->baseSize);
    for (int lvl = 0; lvl < env_specular->levels; lvl++) {
        specular_->load(lvl, env_specular->size(lvl), env_specular->size(lvl), 6, GL_RGB, GL_FLOAT, env_specular->level(lvl).data());
    }

    brdfLut_ = new gl::Texture(GL_TEXTURE_2D);
    brdfLut_->setDebugLabel("ibl_env/environment/brdf_lut");
    brdfLut_->allocate(1, GL_RG32F, brdf_lut->width, brdf_lut->height);
    brdfLut_->load(0, brdf_lut->width, brdf_lut->height, GL_RG, GL_FLOAT, brdf_lut->data.data());
}

IblEnvironment::~IblEnvironment() {
    delete diffuse_;
    delete specular_;
    delete brdfLut_;
    delete cubemapSampler_;
    delete lutSampler_;
}