#pragma once

#include <array>
#include <span>
#include <string>
#include <vector>

#pragma region ForwardDecl
#include "../GL/Declarations.h"
#pragma endregion

namespace loader {

/**
 * An Image Based Lighting Environment
 */
class EnvironmentImage {
   private:
    std::vector<int> sizes_;
    std::vector<float> data_;
    std::vector<std::pair<int32_t, int32_t>> dataByLevel_;
    std::vector<std::array<std::pair<int32_t, int32_t>, 6>> dataByFace_;

   public:
    const int32_t levels;
    const int32_t baseSize;

    EnvironmentImage(std::vector<float> data, int32_t size, int32_t levels);
    ~EnvironmentImage();

    std::vector<float> &all() {
        return data_;
    }

    std::span<float> face(int level, int face) {
        auto range = dataByFace_[level][face];
        return std::span{data_.begin() + range.first, static_cast<size_t>(range.second)};
    }

    std::span<float> level(int level) {
        auto range = dataByLevel_[level];
        return std::span{data_.begin() + range.first, static_cast<size_t>(range.second)};
    }

    int size(int level) {
        return sizes_[level];
    }
};

struct FloatImage {
    std::vector<float> data;
    const int32_t width;
    const int32_t height;
    const int channels;

    FloatImage(int32_t width, int32_t height, int channels) : width(width), height(height), channels(channels) {
    }
};

EnvironmentImage *environment(std::string filename);

FloatImage *floatImage(std::string filename);

class Environment {
    gl::Texture *sky_;
    gl::Texture *diffuse_;
    gl::Texture *specular_;
    gl::Texture *brdfLut_;
    gl::Sampler *cubemapSampler_;
    gl::Sampler *lutSampler_;

   public:
    Environment(
        loader::EnvironmentImage &sky,
        loader::EnvironmentImage &diffuse,
        loader::EnvironmentImage &specular,
        loader::FloatImage &brdf_lut);

    ~Environment();

    gl::Texture &sky() {
        return *sky_;
    }
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

}  // namespace loader
