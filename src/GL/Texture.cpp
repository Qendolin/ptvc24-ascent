#include "Texture.h"

#include <stdexcept>
#include <iostream>

#include "StateManager.h"

namespace GL {

Texture::Texture(GLenum type) : glType(type) {
    glCreateTextures(glType, 1, &glId);
    manager->intelTextureBindingSetTarget(glId, glType);
}

Texture* Texture::as(GLuint glType) {
    return new Texture(glType, glId);
}

void Texture::setDebugLabel(const std::string& label) {
    glObjectLabel(GL_TEXTURE, glId, -1, label.c_str());
}

GLenum Texture::type() const {
    return glType;
}

GLuint Texture::id() const {
    return glId;
}

void Texture::bind(int unit) {
    manager->bindTextureUnit(unit, glId);
}

void Texture::destroy() {
    glDeleteTextures(1, &glId);
    manager->unbindTexture(glId);
    glId = 0;
}

int Texture::dimensions() const {
    switch (glType) {
        case GL_TEXTURE_1D:
        case GL_TEXTURE_BUFFER:
            return 1;
        case GL_TEXTURE_2D:
        case GL_TEXTURE_2D_MULTISAMPLE:
        case GL_TEXTURE_1D_ARRAY:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
            return 2;
        case GL_TEXTURE_3D:
        case GL_TEXTURE_2D_ARRAY:
        case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
        case GL_TEXTURE_CUBE_MAP:
            return 3;
        default:
            std::cerr << "invalid dimension for texture " << std::to_string(glId) << ": " << std::to_string(glType);
            return 0;
    }
}

Texture* Texture::createView(GLenum type, GLenum internalFormat, int minLevel, int maxLevel, int minLayer, int maxLayer) {
    GLuint viewId;
    glGenTextures(1, &viewId);
    glTextureView(viewId, glType, glId, internalFormat, minLevel, maxLevel - minLevel + 1, minLayer, maxLayer - minLayer + 1);
    manager->intelTextureBindingSetTarget(glId, glType);
    return new Texture(glType, viewId);
}

void Texture::allocate(GLint levels, GLuint internalFormat, uint32_t width, uint32_t height, uint32_t depth) {
    if (levels == 0) {
        uint32_t max = std::max(std::max(width, height), depth);
        levels = static_cast<int>(std::log2(max));
        if (levels == 0) {
            levels = 1;
        }
    }
    this->width = width;
    this->height = height;
    this->depth = depth;

    int dim = dimensions();
    if (glType == GL_TEXTURE_CUBE_MAP) {
        dim = 2;
    }

    switch (dim) {
        case 1:
            glTextureStorage1D(glId, levels, internalFormat, width);
            break;
        case 2:
            glTextureStorage2D(glId, levels, internalFormat, width, height);
            break;
        case 3:
            glTextureStorage3D(glId, levels, internalFormat, width, height, depth);
            break;
    }
}

void Texture::allocateMS(GLenum internalFormat, uint32_t width, uint32_t height, uint32_t depth, int samples, bool fixedSampleLocations) {
    this->width = width;
    this->height = height;
    this->depth = depth;
    switch (dimensions()) {
        case 1:
            throw std::runtime_error("1D texture cannot be allocated for multisampling");
        case 2:
            glTextureStorage2DMultisample(glId, samples, internalFormat, width, height, fixedSampleLocations);
            break;
        case 3:
            glTextureStorage3DMultisample(glId, samples, internalFormat, width, height, depth, fixedSampleLocations);
            break;
    }
}

void Texture::load(int level, uint32_t width, uint32_t height, uint32_t depth, GLenum format, GLenum type, void* data) {
    switch (dimensions()) {
        case 1:
            glTextureSubImage1D(glId, level, 0, width, format, type, data);
            break;
        case 2:
            glTextureSubImage2D(glId, level, 0, 0, width, height, format, type, data);
            break;
        case 3:
            glTextureSubImage3D(glId, level, 0, 0, 0, width, height, depth, format, type, data);
            break;
    }
}

void Texture::generateMipmap() {
    glGenerateTextureMipmap(glId);
}

void Texture::mipmapLevels(int base, int max) {
    glTextureParameteri(glId, GL_TEXTURE_BASE_LEVEL, base);
    glTextureParameteri(glId, GL_TEXTURE_MAX_LEVEL, max);
}

void Texture::depthStencilTextureMode(GLenum mode) {
    glTextureParameteri(glId, GL_DEPTH_STENCIL_TEXTURE_MODE, mode);
}

Sampler::Sampler() {
    glCreateSamplers(1, &glId);
}

void Sampler::destroy() {
    glDeleteSamplers(1, &glId);
    manager->unbindSampler(glId);
    glId = 0;
}

GLuint Sampler::Sampler::id() const {
    return glId;
}

void Sampler::setDebugLabel(const std::string& label) {
    glObjectLabel(GL_SAMPLER, glId, -1, label.c_str());
}

void Sampler::bind(int unit) const {
    manager->bindSampler(unit, glId);
}

void Sampler::filterMode(GLenum min, GLenum mag) {
    if (min != 0) {
        glSamplerParameteri(glId, GL_TEXTURE_MIN_FILTER, min);
    }
    if (mag != 0) {
        glSamplerParameteri(glId, GL_TEXTURE_MAG_FILTER, mag);
    }
}

void Sampler::wrapMode(GLenum s, GLenum t, GLenum r) {
    if (s != 0) {
        glSamplerParameteri(glId, GL_TEXTURE_WRAP_S, s);
    }
    if (t != 0) {
        glSamplerParameteri(glId, GL_TEXTURE_WRAP_T, t);
    }
    if (r != 0) {
        glSamplerParameteri(glId, GL_TEXTURE_WRAP_R, r);
    }
}

void Sampler::compareMode(GLenum mode, GLenum fn) {
    glSamplerParameteri(glId, GL_TEXTURE_COMPARE_MODE, mode);
    if (fn != 0) {
        glSamplerParameteri(glId, GL_TEXTURE_COMPARE_FUNC, fn);
    }
}

void Sampler::borderColor(glm::vec4 color) {
    glSamplerParameterfv(glId, GL_TEXTURE_BORDER_COLOR, &color[0]);
}

void Sampler::anisotropicFilter(float quality) {
    glSamplerParameterf(glId, GL_TEXTURE_MAX_ANISOTROPY, quality);
}

void Sampler::lodBias(float bias) {
    glSamplerParameterf(glId, GL_TEXTURE_LOD_BIAS, bias);
}

}  // namespace GL
