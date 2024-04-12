#pragma once

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "../GL/Declarations.h"

namespace loader {

struct Image {
    int width = 0;
    int height = 0;
    int channels = 4;
    std::shared_ptr<uint8_t> data;
};

std::string text(std::string filename);

std::ifstream stream(std::string filename);

std::vector<uint8_t> binary(std::string filename);

loader::Image image(std::string filename);

typedef unsigned int GLenum;

struct TextureParameters {
    bool mipmap = true;
    bool srgb = false;
    GLenum internalFormat = 0x8058 /* GL_RGBA8 */;
    GLenum fileFormat = 0x1908 /* GL_RGBA */;
    GLenum dataType = 0x1401 /* GL_UNSIGNED_BYTE */;
};

gl::Texture *texture(std::string filename, TextureParameters params = {});

}  // namespace loader
