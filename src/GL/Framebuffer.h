#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "Object.h"

namespace gl {

class Texture;

// References;
// https://www.khronos.org/opengl/wiki/Renderbuffer_Object
class Renderbuffer : public GLObject {
   public:
    Renderbuffer();
    virtual ~Renderbuffer();

    Renderbuffer(Renderbuffer&&) noexcept = default;

    void bind() const;

    void allocate(GLenum internalFormat, uint32_t width, uint32_t height);

    void allocateMS(GLenum internalFormat, uint32_t width, uint32_t height, int samples);
};

// References:
// https://www.khronos.org/opengl/wiki/Framebuffer
// https://www.khronos.org/opengl/wiki/Framebuffer_Object
class Framebuffer : public GLObject {
   private:
    std::vector<Texture*> textures_;
    std::vector<Renderbuffer*> renderbuffers_;

    // maps an attachment to it's index in the vector
    int mapAttachmentIndex_(int attachment) const;

   public:
    Framebuffer();
    virtual ~Framebuffer();

    Framebuffer(Framebuffer&&) noexcept = default;

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glDrawBuffers.xhtml)
    void bindTargets(const std::vector<int>& indices);

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glCheckFramebufferStatus.xhtml)
    void check(GLenum target) const;

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBindFramebuffer.xhtml)
    void bind(GLenum target) const;

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glFramebufferTexture.xhtml)
    void attachTexture(int index, Texture* texture);

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glFramebufferTexture.xhtml)
    void attachTextureLevel(int index, Texture* texture, int level);

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glFramebufferTextureLayer.xhtml)
    void attachTextureLayer(int index, Texture* texture, int layer);

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glFramebufferTextureLayer.xhtml)
    void attachTextureLayerLevel(int index, Texture* texture, int layer, int level);

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glFramebufferRenderbuffer.xhtml)
    void attachRenderbuffer(int index, Renderbuffer* renderbuffer);

    /**
     * @param index either an attachment index or one of GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT, GL_DEPTH_STENCIL_ATTACHMENT
     */
    Texture* getTexture(int index) const;

    /**
     * @param index either an attachment index or one of GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT, GL_DEPTH_STENCIL_ATTACHMENT
     */
    Renderbuffer* getRenderbuffer(int index) const;
};

}  // namespace gl
