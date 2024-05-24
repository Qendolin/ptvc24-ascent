#pragma once
#include <array>
#include <span>
#include <string>
#include <vector>

namespace loader {

/**
 * An Image Based Lighting Environment
 */
class IblEnv {
   private:
    std::vector<int> sizes_;
    std::vector<float> data_;
    std::vector<std::pair<int32_t, int32_t>> dataByLevel_;
    std::vector<std::array<std::pair<int32_t, int32_t>, 6>> dataByFace_;

   public:
    const int32_t levels;
    const int32_t baseSize;

    IblEnv(std::vector<float> data, int32_t size, int32_t levels);

    std::vector<float>& all() {
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

IblEnv* environment(std::string filename);

FloatImage* floatImage(std::string filename);

}  // namespace loader
