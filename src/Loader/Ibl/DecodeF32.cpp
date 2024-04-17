#include "../Environment.h"
//

#include <fstream>
#include <vector>

#include "../../Utils.h"

const uint32_t F32_MAGIC_NUMBER = 0x6d16837d;
const uint32_t F32_VERSION_1_002_001 = 1002001;

// force tight packing
#pragma pack(push, 1)
struct FloatImageHeader {
    uint32_t check;
    uint32_t version;
    uint32_t width;
    uint32_t height;
    uint8_t channels;
    uint8_t compression;
    uint8_t unused[14];
};
#pragma pack(pop)

std::vector<uint8_t> decompressLz4Frames(std::vector<uint8_t>& input);
loader::FloatImage* decodeF32(std::ifstream& input);

namespace loader {
FloatImage* floatImage(std::string filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        PANIC("Error opening file: " + filename);
    }
    return decodeF32(file);
}
}  // namespace loader

loader::FloatImage* decodeF32(std::ifstream& input) {
    FloatImageHeader header;
    input.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (input.fail() || header.check != F32_MAGIC_NUMBER) {
        PANIC("Expected F32 header");
    }

    if (header.version != F32_VERSION_1_002_001) {
        PANIC("F32 version unsupported");
    }

    if (header.compression != 0) {
        PANIC("F32 compression unsupported");
    }

    size_t expected_floats = header.width * header.height * header.channels;
    auto result = new loader::FloatImage(header.width, header.height, header.channels);
    result->data.resize(expected_floats);

    input.read(reinterpret_cast<char*>(result->data.data()), result->data.size() * 4);

    size_t bytes_read = static_cast<size_t>(input.gcount());
    if (bytes_read != 4 * expected_floats) {
        PANIC("Expected " + std::to_string(expected_floats) + " floats");
    }

    return result;
}
