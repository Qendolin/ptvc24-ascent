#include "../Environment.h"
//

#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>

#include "../../Util/Log.h"
#include "LZ4.h"

const uint32_t IBLENV_MAGIC_NUMBER = 0x78b85411;
const uint32_t IBLENV_VERSION_1_002_000 = 1002000;
const uint32_t IBLENV_COMPRESSION_NONE = 0;
const uint32_t IBLENV_COMPRESSION_LZ4 = 2;

struct IblEnvHeader {
    uint32_t check;
    uint32_t version;
    uint32_t compression;
    uint32_t size;
    uint32_t levels;
};

size_t decodeRgbeChunk(std::vector<uint8_t>& src, std::vector<float>& dst);

loader::IblEnv* decodeIblEnv(std::istream& input);

uint32_t calcCubeMapPixels(uint32_t size, uint32_t levels);

namespace loader {
IblEnv* environment(std::string filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        PANIC("Error opening file: " + filename);
    }
    return decodeIblEnv(file);
}
}  // namespace loader

loader::IblEnv* decodeIblEnv(std::istream& input) {
    IblEnvHeader header;
    input.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (input.fail() || header.check != IBLENV_MAGIC_NUMBER) {
        PANIC("Expected environment header");
    }

    if (header.version != IBLENV_VERSION_1_002_000) {
        PANIC("Environment version unsupported");
    }

    uint32_t pixel_count = calcCubeMapPixels(header.size, header.levels);
    std::vector<uint8_t> rgbe_data;

    if (header.compression == IBLENV_COMPRESSION_NONE) {
        rgbe_data.resize(pixel_count * 4);
        input.read(reinterpret_cast<char*>(rgbe_data.data()), rgbe_data.size());
        if (input.gcount() != pixel_count * 4) {
            PANIC("Expected " + std::to_string(pixel_count) + " encoded pixels");
        }
    } else if (header.compression == IBLENV_COMPRESSION_LZ4) {
        rgbe_data = decompressLz4Frames(input);
        if (rgbe_data.size() != pixel_count * 4) {
            PANIC("Expected " + std::to_string(pixel_count) + " encoded pixels");
        }
    } else {
        PANIC("Environment compression method unsupported");
    }

    std::vector<float> result;
    result.resize(pixel_count * 3);
    size_t decoded = decodeRgbeChunk(rgbe_data, result);
    if (decoded != pixel_count * 3) {
        PANIC("Did not decode as many RGBE pixels as expected");
    }

    return new loader::IblEnv(result, header.size, header.levels);
}

uint32_t calcCubeMapPixels(uint32_t size, uint32_t levels) {
    uint32_t sum = size * size * 6;
    for (uint32_t i = 1; i < levels; i++) {
        size /= 2;
        sum += size * size * 6;
        if (size == 1) {
            break;
        }
    }
    return sum;
}

// https://github.com/JuliaMath/openlibm/blob/12f5ffcc990e16f4120d4bf607185243f5affcb8/src/math_private.h#L161C1-L161C1
typedef union {
    float value;
    unsigned int word;
} ieee_float_shape_type;

// https://github.com/KnightOS/libc/blob/c1ab6948303ada88f6ab0923035413b8be92a209/src/gpl/ldexpf.c
static inline float fast_ldexpf(float a, int pw2) {
    ieee_float_shape_type fl;
    uint32_t e;

    fl.value = a;

    e = (fl.word >> 23) & 0x000000ff;
    e += pw2;
    fl.word = ((e & 0xff) << 23) | (fl.word & 0x807fffff);

    return (fl.value);
}

size_t decodeRgbeChunk(std::vector<uint8_t>& src, std::vector<float>& dst) {
    size_t n = 0;
    for (uint32_t i = 0, j = 0; i < src.size(); i += 4, j += 3) {
        uint8_t r = src[i + 0];
        uint8_t g = src[i + 1];
        uint8_t b = src[i + 2];
        uint8_t e = src[i + 3];

        if (e == 0.0f) {
            dst[j + 0] = 0.0;
            dst[j + 1] = 0.0;
            dst[j + 2] = 0.0;
            n += 3;
            continue;
        }

        float f = fast_ldexpf(1.0, (int)(e) - (128 + 8));

        dst[j + 0] = (float)(r)*f;
        dst[j + 1] = (float)(g)*f;
        dst[j + 2] = (float)(b)*f;
        n += 3;
    }

    return n;
}
