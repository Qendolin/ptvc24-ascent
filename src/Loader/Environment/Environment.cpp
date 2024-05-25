#include "../Environment.h"

#include "../../GL/Texture.h"

std::pair<int32_t, int32_t> calcCubeMapOffset(int32_t size, int32_t level) {
    int32_t start = 0, end = 0;
    for (int32_t i = 0; i <= level; i++) {
        int32_t len = size * size * 6;
        end += len;
        start = end - len;
        size /= 2;
        if (size == 0) {
            break;
        }
    }
    return std::make_pair(start, end);
}

namespace loader {
EnvironmentImage::EnvironmentImage(std::vector<float> data, int32_t size, int32_t levels) : levels(levels), baseSize(size) {
    dataByLevel_.resize(levels);
    dataByFace_.resize(levels);
    sizes_.resize(levels);
    int32_t lvl_size = size;

    std::span<float> data_span = std::span{data};

    for (int lvl = 0; lvl < levels; lvl++) {
        int32_t offset, end;
        std::tie(offset, end) = calcCubeMapOffset(size, lvl);
        offset *= 3;
        end *= 3;
        int32_t stride = lvl_size * lvl_size * 3;

        dataByFace_[lvl] = {
            std::make_pair(offset + 0 * stride, stride),
            std::make_pair(offset + 1 * stride, stride),
            std::make_pair(offset + 2 * stride, stride),
            std::make_pair(offset + 3 * stride, stride),
            std::make_pair(offset + 4 * stride, stride),
            std::make_pair(offset + 5 * stride, stride)};

        dataByLevel_[lvl] = std::make_pair(offset, end - offset);
        sizes_[lvl] = lvl_size;
        lvl_size /= 2;
    }

    this->data_ = data;
}

EnvironmentImage::~EnvironmentImage() {
}

Environment::Environment(
    loader::EnvironmentImage& sky,
    loader::EnvironmentImage& env_diffuse,
    loader::EnvironmentImage& env_specular,
    loader::FloatImage& brdf_lut) {
    lutSampler_ = new gl::Sampler();
    lutSampler_->setDebugLabel("environment/lut_sampler");
    lutSampler_->wrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    lutSampler_->filterMode(GL_LINEAR, GL_LINEAR);

    cubemapSampler_ = new gl::Sampler();
    cubemapSampler_->setDebugLabel("environment/cubemap_sampler");
    cubemapSampler_->wrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    cubemapSampler_->filterMode(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

    diffuse_ = new gl::Texture(GL_TEXTURE_CUBE_MAP);
    diffuse_->setDebugLabel("environment/diffuse");
    diffuse_->allocate(1, GL_RGB16F, env_diffuse.baseSize, env_diffuse.baseSize);
    diffuse_->load(0, env_diffuse.baseSize, env_diffuse.baseSize, 6, GL_RGB, GL_FLOAT, env_diffuse.all().data());

    specular_ = new gl::Texture(GL_TEXTURE_CUBE_MAP);
    specular_->setDebugLabel("environment/diffuse");
    specular_->allocate(env_specular.levels, GL_RGB16F, env_specular.baseSize, env_specular.baseSize);
    for (int lvl = 0; lvl < env_specular.levels; lvl++) {
        specular_->load(lvl, env_specular.size(lvl), env_specular.size(lvl), 6, GL_RGB, GL_FLOAT, env_specular.level(lvl).data());
    }

    brdfLut_ = new gl::Texture(GL_TEXTURE_2D);
    brdfLut_->setDebugLabel("environment/brdf_lut");
    brdfLut_->allocate(1, GL_RG32F, brdf_lut.width, brdf_lut.height);
    brdfLut_->load(0, brdf_lut.width, brdf_lut.height, GL_RG, GL_FLOAT, brdf_lut.data.data());

    sky_ = new gl::Texture(GL_TEXTURE_CUBE_MAP);
    sky_->setDebugLabel("environment/sky");
    sky_->allocate(1, GL_RGB16F, sky.baseSize, sky.baseSize);
    sky_->load(0, sky.baseSize, sky.baseSize, 6, GL_RGB, GL_FLOAT, sky.all().data());
}

Environment::~Environment() {
    delete diffuse_;
    delete specular_;
    delete brdfLut_;
    delete cubemapSampler_;
    delete lutSampler_;
    delete sky_;
}

}  // namespace loader
