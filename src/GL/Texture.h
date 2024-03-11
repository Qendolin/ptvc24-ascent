#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "Object.h"

namespace GL {

class Texture : public GLObject {
   private:
    GLuint id_ = 0;
    GLenum type_ = 0;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    uint32_t depth_ = 0;

    Texture(GLenum type, GLuint id) : type_(type), id_(id) {
    }

   public:
    Texture(GLenum type);

    ~Texture() {
        checkDestroyed(GL_TEXTURE);
    }

    Texture* as(GLuint glType);

    void setDebugLabel(const std::string& label);

    GLenum type() const;

    int dimensions() const;

    GLuint id() const;

    void bind(int unit);

    void destroy();

    Texture* createView(GLenum type, GLenum internalFormat, int minLevel, int maxLevel, int minLayer, int maxLayer);

    void allocate(GLint levels, GLuint internalFormat, uint32_t width, uint32_t height, uint32_t depth);

    void allocateMS(GLenum internalFormat, uint32_t width, uint32_t height, uint32_t depth, int samples, bool fixedSampleLocations);

    void load(int level, uint32_t width, uint32_t height, uint32_t depth, GLenum format, GLenum type, void* data);

    void generateMipmap();

    void mipmapLevels(int base, int max);

    void depthStencilTextureMode(GLenum mode);
};

class Sampler : public GLObject {
   private:
    GLuint id_ = 0;

   public:
    Sampler();

    ~Sampler() {
        checkDestroyed(GL_SAMPLER);
    }

    void destroy();

    GLuint id() const;

    void setDebugLabel(const std::string& label);

    void bind(int unit) const;

    void filterMode(GLenum min, GLenum mag);

    void wrapMode(GLenum s, GLenum t, GLenum r);

    void compareMode(GLenum mode, GLenum fn);

    void borderColor(glm::vec4 color);

    void anisotropicFilter(float quality);

    void lodBias(float bias);
};

}  // namespace GL
