#include "../Environment.h"

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
IblEnv::IblEnv(std::vector<float> data, int32_t size, int32_t levels) : levels(levels), baseSize(size) {
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

}  // namespace loader
