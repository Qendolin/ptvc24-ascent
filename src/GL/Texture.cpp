#include "Texture.h"

#include <iostream>
#include <stdexcept>

#include "StateManager.h"

namespace GL {

Texture::Texture(GLenum type) : type_(type) {
    glCreateTextures(type_, 1, &id_);
    manager->intelTextureBindingSetTarget(id_, type_);
}

Texture* Texture::as(GLuint glType) {
    return new Texture(glType, id_);
}

void Texture::setDebugLabel(const std::string& label) {
    glObjectLabel(GL_TEXTURE, id_, -1, label.c_str());
}

GLenum Texture::type() const {
    return type_;
}

GLuint Texture::id() const {
    return id_;
}

void Texture::bind(int unit) {
    manager->bindTextureUnit(unit, id_);
}

void Texture::destroy() {
    if (id_ != 0) {
        glDeleteTextures(1, &id_);
        manager->unbindTexture(id_);
        id_ = 0;
    }
    delete this;
}

int Texture::dimensions() const {
    switch (type_) {
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
            std::cerr << "invalid dimension for texture " << std::to_string(id_) << ": " << std::to_string(type_);
            return 0;
    }
}

Texture* Texture::createView(GLenum type, GLenum internalFormat, int minLevel, int maxLevel, int minLayer, int maxLayer) {
    GLuint viewId;
    glGenTextures(1, &viewId);
    glTextureView(viewId, type_, id_, internalFormat, minLevel, maxLevel - minLevel + 1, minLayer, maxLayer - minLayer + 1);
    manager->intelTextureBindingSetTarget(id_, type_);
    return new Texture(type_, viewId);
}

void Texture::allocate(GLint levels, GLenum internalFormat, uint32_t width, uint32_t height, uint32_t depth) {
    if (levels == 0) {
        uint32_t max = std::max(std::max(width, height), depth);
        levels = static_cast<int>(std::log2(max));
        if (levels == 0) {
            levels = 1;
        }
    }
    this->width_ = width;
    this->height_ = height;
    this->depth_ = depth;

    int dim = dimensions();
    if (type_ == GL_TEXTURE_CUBE_MAP) {
        dim = 2;
    }

    switch (dim) {
        case 1:
            glTextureStorage1D(id_, levels, internalFormat, width);
            break;
        case 2:
            glTextureStorage2D(id_, levels, internalFormat, width, height);
            break;
        case 3:
            glTextureStorage3D(id_, levels, internalFormat, width, height, depth);
            break;
    }
}

void Texture::allocateMS(GLenum internalFormat, uint32_t width, uint32_t height, uint32_t depth, int samples, bool fixedSampleLocations) {
    this->width_ = width;
    this->height_ = height;
    this->depth_ = depth;
    switch (dimensions()) {
        case 1:
            throw std::runtime_error("1D texture cannot be allocated for multisampling");
        case 2:
            glTextureStorage2DMultisample(id_, samples, internalFormat, width, height, fixedSampleLocations);
            break;
        case 3:
            glTextureStorage3DMultisample(id_, samples, internalFormat, width, height, depth, fixedSampleLocations);
            break;
    }
}

void Texture::load(int level, uint32_t width, uint32_t height, uint32_t depth, GLenum format, GLenum type, const void* data) {
    switch (dimensions()) {
        case 1:
            glTextureSubImage1D(id_, level, 0, width, format, type, data);
            break;
        case 2:
            glTextureSubImage2D(id_, level, 0, 0, width, height, format, type, data);
            break;
        case 3:
            glTextureSubImage3D(id_, level, 0, 0, 0, width, height, depth, format, type, data);
            break;
    }
}

void Texture::generateMipmap() {
    glGenerateTextureMipmap(id_);
}

void Texture::mipmapLevels(int base, int max) {
    glTextureParameteri(id_, GL_TEXTURE_BASE_LEVEL, base);
    glTextureParameteri(id_, GL_TEXTURE_MAX_LEVEL, max);
}

void Texture::depthStencilTextureMode(GLenum mode) {
    glTextureParameteri(id_, GL_DEPTH_STENCIL_TEXTURE_MODE, mode);
}

Sampler::Sampler() {
    glCreateSamplers(1, &id_);
}

void Sampler::destroy() {
    if (id_ != 0) {
        glDeleteSamplers(1, &id_);
        manager->unbindSampler(id_);
        id_ = 0;
    }
    delete this;
}

GLuint Sampler::Sampler::id() const {
    return id_;
}

void Sampler::setDebugLabel(const std::string& label) {
    glObjectLabel(GL_SAMPLER, id_, -1, label.c_str());
}

void Sampler::bind(int unit) const {
    manager->bindSampler(unit, id_);
}

void Sampler::filterMode(GLenum min, GLenum mag) {
    if (min != 0) {
        glSamplerParameteri(id_, GL_TEXTURE_MIN_FILTER, min);
    }
    if (mag != 0) {
        glSamplerParameteri(id_, GL_TEXTURE_MAG_FILTER, mag);
    }
}

void Sampler::wrapMode(GLenum s, GLenum t, GLenum r) {
    if (s != 0) {
        glSamplerParameteri(id_, GL_TEXTURE_WRAP_S, s);
    }
    if (t != 0) {
        glSamplerParameteri(id_, GL_TEXTURE_WRAP_T, t);
    }
    if (r != 0) {
        glSamplerParameteri(id_, GL_TEXTURE_WRAP_R, r);
    }
}

void Sampler::compareMode(GLenum mode, GLenum fn) {
    glSamplerParameteri(id_, GL_TEXTURE_COMPARE_MODE, mode);
    if (fn != 0) {
        glSamplerParameteri(id_, GL_TEXTURE_COMPARE_FUNC, fn);
    }
}

void Sampler::borderColor(glm::vec4 color) {
    glSamplerParameterfv(id_, GL_TEXTURE_BORDER_COLOR, &color[0]);
}

void Sampler::anisotropicFilter(float quality) {
    glSamplerParameterf(id_, GL_TEXTURE_MAX_ANISOTROPY, quality);
}

void Sampler::lodBias(float bias) {
    glSamplerParameterf(id_, GL_TEXTURE_LOD_BIAS, bias);
}

}  // namespace GL
