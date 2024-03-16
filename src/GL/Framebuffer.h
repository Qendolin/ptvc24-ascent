#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "Object.h"
#include "Texture.h"

namespace GL {

class Renderbuffer : public GLObject {
   private:
    GLuint id_ = 0;

    ~Renderbuffer() {
        checkDestroyed(GL_RENDERBUFFER);
    }

   public:
    Renderbuffer();

    void destroy();

    GLuint id() const;

    void setDebugLabel(const std::string& label);

    void bind() const;

    void allocate(GLenum internalFormat, uint32_t width, uint32_t height);

    void allocateMS(GLenum internalFormat, uint32_t width, uint32_t height, int samples);
};

class Framebuffer : public GLObject {
   private:
    GLuint id_ = 0;
    std::vector<const Texture*> textures_;
    std::vector<const Renderbuffer*> renderbuffers_;

    int mapAttachmentIndex_(int index) const;

    ~Framebuffer() {
        checkDestroyed(GL_FRAMEBUFFER);
    }

   public:
    Framebuffer();

    void destroy();

    GLuint id() const;

    void setDebugLabel(const std::string& label);

    void bindTargets(const std::vector<int>& indices);

    void check(GLenum target) const;

    void bind(GLenum target) const;

    void attachTexture(int index, const Texture* texture);

    void attachTextureLevel(int index, const Texture* texture, int level);

    void attachTextureLayer(int index, const Texture* texture, int layer);

    void attachTextureLayerLevel(int index, const Texture* texture, int layer, int level);

    void attachRenderbuffer(int index, const Renderbuffer* renderbuffer);

    const Texture* getTexture(int index) const;

    const Renderbuffer* getRenderbuffer(int index) const;
};

}  // namespace GL
