#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "Object.h"
#include "Texture.h"

namespace GL {

// References;
// https://www.khronos.org/opengl/wiki/Renderbuffer_Object
class Renderbuffer : public GLObject {
   public:
    Renderbuffer();

    Renderbuffer(Renderbuffer&&) noexcept = default;

    void destroy() override;

    void setDebugLabel(const std::string& label) override;

    void bind() const;

    void allocate(GLenum internalFormat, uint32_t width, uint32_t height);

    void allocateMS(GLenum internalFormat, uint32_t width, uint32_t height, int samples);
};

// References:
// https://www.khronos.org/opengl/wiki/Framebuffer
// https://www.khronos.org/opengl/wiki/Framebuffer_Object
class Framebuffer : public GLObject {
   private:
    std::vector<const Texture*> textures_;
    std::vector<const Renderbuffer*> renderbuffers_;

    // maps an attachment to it's index in the vector
    int mapAttachmentIndex_(int attachment) const;

   public:
    Framebuffer();

    Framebuffer(Framebuffer&&) noexcept = default;

    void destroy() override;

    void setDebugLabel(const std::string& label) override;

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glDrawBuffers.xhtml)
    void bindTargets(const std::vector<int>& indices);

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glCheckFramebufferStatus.xhtml)
    void check(GLenum target) const;

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBindFramebuffer.xhtml)
    void bind(GLenum target) const;

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glFramebufferTexture.xhtml)
    void attachTexture(int index, const Texture* texture);

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glFramebufferTexture.xhtml)
    void attachTextureLevel(int index, const Texture* texture, int level);

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glFramebufferTextureLayer.xhtml)
    void attachTextureLayer(int index, const Texture* texture, int layer);

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glFramebufferTextureLayer.xhtml)
    void attachTextureLayerLevel(int index, const Texture* texture, int layer, int level);

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glFramebufferRenderbuffer.xhtml)
    void attachRenderbuffer(int index, const Renderbuffer* renderbuffer);

    const Texture* getTexture(int index) const;

    const Renderbuffer* getRenderbuffer(int index) const;
};

}  // namespace GL
