#pragma once

#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "../GL/Geometry.h"
#include "../GL/Texture.h"

namespace Asset {

typedef struct Image {
    int width = 0;
    int height = 0;
    int channels = 4;
    std::shared_ptr<uint8_t> data;
} Image;

}  // namespace Asset

namespace Loader {

std::string text(std::string filename);

std::ifstream stream(std::string filename);

std::vector<uint8_t> binary(std::string filename);

Asset::Image image(std::string filename);

struct TextureParameters {
    bool mipmap = true;
    bool srgb = false;
    GLenum internalFormat = GL_RGBA8;
    GLenum fileFormat = GL_RGBA;
    GLenum dataType = GL_UNSIGNED_BYTE;
};

GL::Texture *texture(std::string filename, TextureParameters params = {});

}  // namespace Loader
