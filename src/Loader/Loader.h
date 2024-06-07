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
    size_t length = 0;
    std::shared_ptr<uint8_t> data;
};

typedef std::vector<std::vector<std::string>> CSV;

std::string text(std::string filename);

CSV csv(std::string filename);

void writeCsv(std::string filename, CSV &csv);

std::ifstream stream(std::string filename);

std::vector<uint8_t> binary(std::string filename);

loader::Image image(std::string filename);

void writeImage(std::string filename, loader::Image image);

typedef unsigned int GLenum;

struct TextureParameters {
    bool mipmap = true;
    bool srgb = false;
    GLenum internalFormat = 0x8058 /* GL_RGBA8 */;
    GLenum fileFormat = 0x1908 /* GL_RGBA */;
    GLenum dataType = 0x1401 /* GL_UNSIGNED_BYTE */;
};

gl::Texture *texture(std::string filename, TextureParameters params = {});

gl::Texture *texture(loader::Image &image, TextureParameters params = {});

}  // namespace loader
