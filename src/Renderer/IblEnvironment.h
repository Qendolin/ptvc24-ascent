#pragma once

#include <memory>

#pragma region ForwardDecl
#include "../GL/Declarations.h"
#pragma endregion

#include "../Loader/Environment.h"

class IblEnvironment {
    gl::Texture *diffuse_;
    gl::Texture *specular_;
    gl::Texture *brdfLut_;
    gl::Sampler *cubemapSampler_;
    gl::Sampler *lutSampler_;

   public:
    IblEnvironment(std::shared_ptr<loader::IblEnv> diffuse,
                   std::shared_ptr<loader::IblEnv> specular,
                   std::shared_ptr<loader::FloatImage> brdf_lut);

    ~IblEnvironment();

    gl::Texture &diffuse() {
        return *diffuse_;
    }
    gl::Texture &specular() {
        return *specular_;
    }
    gl::Texture &brdfLut() {
        return *brdfLut_;
    }
    gl::Sampler &cubemapSampler() {
        return *cubemapSampler_;
    }
    gl::Sampler &lutSampler() {
        return *lutSampler_;
    }
};