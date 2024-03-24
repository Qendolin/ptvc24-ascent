#include "Loader.h"

#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_HDR
#define STBI_ONLY_BMP
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

#include "../Utils.h"

namespace Loader {

Asset::Image image(std::string filename) {
    std::vector<uint8_t> data = binary(filename);

    int w, h, n;
    uint8_t *image = stbi_load_from_memory(data.data(), data.size(), &w, &h, &n, 4);
    if (image == nullptr) {
        PANIC("Error loading image: " + filename + ", reason: " + stbi_failure_reason());
    }

    return Asset::Image{
        .width = w,
        .height = h,
        .channels = 4,
        .data = std::shared_ptr<uint8_t>(image),
    };
}

GL::Texture *texture(std::string filename) {
    Asset::Image img = image(filename);
    GL::Texture *texture = new GL::Texture(GL_TEXTURE_2D);
    texture->allocate(0, GL_RGBA8, img.width, img.height, 1);
    texture->load(0, img.width, img.height, 1, GL_RGBA, GL_UNSIGNED_BYTE, img.data.get());
    texture->generateMipmap();
    return texture;
}

}  // namespace Loader
