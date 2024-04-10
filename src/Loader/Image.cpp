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

Loader::Image image(std::string filename) {
    std::vector<uint8_t> data = binary(filename);

    int w, h, n;
    uint8_t *image = stbi_load_from_memory(data.data(), data.size(), &w, &h, &n, 4);
    if (image == nullptr) {
        PANIC("Error loading image: " + filename + ", reason: " + stbi_failure_reason());
    }

    return Loader::Image{
        .width = w,
        .height = h,
        .channels = 4,
        .data = std::shared_ptr<uint8_t>(image),
    };
}

GL::Texture *texture(std::string filename, TextureParameters params) {
    Loader::Image img = image(filename);
    GL::Texture *texture = new GL::Texture(GL_TEXTURE_2D);
    GLenum internal_format = params.internalFormat;
    if (params.srgb) {
        if (params.internalFormat == GL_RGB8 || params.internalFormat == GL_SRGB8)
            internal_format = GL_SRGB8;
        else if (params.internalFormat == GL_RGBA8 || params.internalFormat == GL_SRGB8_ALPHA8)
            internal_format = GL_SRGB8_ALPHA8;
        else
            PANIC("Invalid texture params");
    }
    texture->allocate(params.mipmap ? 0 : 1, params.internalFormat, img.width, img.height, 1);
    texture->load(0, img.width, img.height, 1, params.fileFormat, params.dataType, img.data.get());
    if (params.mipmap)
        texture->generateMipmap();
    return texture;
}

}  // namespace Loader
