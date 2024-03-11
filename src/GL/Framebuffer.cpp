#include "Framebuffer.h"

#include <stdexcept>

#include "../Utils.h"
#include "StateManager.h"

namespace GL {

Renderbuffer::Renderbuffer() {
    glCreateRenderbuffers(1, &id_);
}

void Renderbuffer::destroy() {
    if (id_ != 0) {
        glDeleteRenderbuffers(1, &id_);
        manager->unbindRenderbuffer(id_);
        id_ = 0;
    }
    delete this;
}

GLuint Renderbuffer::id() const {
    return id_;
}

void Renderbuffer::setDebugLabel(const std::string& label) {
    glObjectLabel(GL_RENDERBUFFER, id_, -1, label.c_str());
}

void Renderbuffer::bind() const {
    manager->bindRenderbuffer(id_);
}

void Renderbuffer::allocate(GLenum internalFormat, uint32_t width, uint32_t height) {
    glNamedRenderbufferStorage(id_, internalFormat, width, height);
}

void Renderbuffer::allocateMS(GLenum internalFormat, uint32_t width, uint32_t height, int samples) {
    glNamedRenderbufferStorageMultisample(id_, samples, internalFormat, width, height);
}

const int MAX_ATTACHMENTS = 8;

int Framebuffer::mapAttachmentIndex(int index) const {
    if (index == GL_DEPTH_ATTACHMENT || index == GL_DEPTH_STENCIL_ATTACHMENT) {
        return 0;
    } else if (index == GL_STENCIL_ATTACHMENT) {
        return 1;
    }
    return index + 2;
}

Framebuffer::Framebuffer() : textures_(MAX_ATTACHMENTS + 2, nullptr), renderbuffers_(MAX_ATTACHMENTS + 2, nullptr) {
    glCreateFramebuffers(1, &id_);
}

void Framebuffer::destroy() {
    if (id_ != 0) {
        glDeleteFramebuffers(1, &id_);
        manager->unbindFramebuffer(id_);
        id_ = 0;
    }
    delete this;
}

GLuint Framebuffer::id() const {
    return id_;
}

void Framebuffer::setDebugLabel(const std::string& label) {
    glObjectLabel(GL_FRAMEBUFFER, id_, -1, label.c_str());
}

void Framebuffer::bindTargets(const std::vector<int>& indices) {
    std::vector<GLenum> attachments(indices.size());
    for (int i = 0; i < indices.size(); i++) {
        int index = indices[i];
        if (index <= MAX_ATTACHMENTS) {
            attachments[i] = GL_COLOR_ATTACHMENT0 + index;
        } else {
            attachments[i] = index;
        }
    }
    glNamedFramebufferDrawBuffers(id_, attachments.size(), attachments.data());
}

void Framebuffer::check(GLenum target) const {
    GLenum status = glCheckNamedFramebufferStatus(id_, target);
    switch (status) {
        case GL_FRAMEBUFFER_COMPLETE:
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            throw std::runtime_error("An attachment is framebuffer incomplete (GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT)");
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            throw std::runtime_error("The framebuffer has no attachments (GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT)");
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
            throw std::runtime_error("The object type of a draw attachment is none (GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER)");
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
            throw std::runtime_error("The object type of the read attachment is none (GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER)");
        case GL_FRAMEBUFFER_UNSUPPORTED:
            throw std::runtime_error("The combination of internal formats of the attachments is not supported (GL_FRAMEBUFFER_UNSUPPORTED)");
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
            throw std::runtime_error("The attachments have different sampling (GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE)");
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
            throw std::runtime_error("FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS");
        default:
            throw std::runtime_error("Unknown framebuffer status: " + std::to_string(status));
    }
}

void Framebuffer::bind(GLenum target) const {
    manager->bindFramebuffer(target, id_);
}

void Framebuffer::attachTexture(int index, const Texture* texture) {
    attachTextureLevel(index, texture, 0);
}

void Framebuffer::attachTextureLevel(int index, const Texture* texture, int level) {
    textures_[mapAttachmentIndex(index)] = texture;
    if (index <= MAX_ATTACHMENTS) {
        index += GL_COLOR_ATTACHMENT0;
    }
    glNamedFramebufferTexture(id_, index, texture->id(), level);
}

void Framebuffer::attachTextureLayer(int index, const Texture* texture, int layer) {
    attachTextureLayerLevel(index, texture, layer, 0);
}

void Framebuffer::attachTextureLayerLevel(int index, const Texture* texture, int layer, int level) {
    textures_[mapAttachmentIndex(index)] = texture;
    if (index <= MAX_ATTACHMENTS) {
        index += GL_COLOR_ATTACHMENT0;
    }
    // https://community.intel.com/t5/Graphics/glNamedFramebufferTextureLayer-rejects-cubemaps-of-any-kind/td-p/1167643
    if (texture->type() == GL_TEXTURE_CUBE_MAP && manager->environment().useIntelCubemapDsaFix) {
        glBindFramebuffer(GL_FRAMEBUFFER, id_);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texture->id());
        glFramebufferTexture2D(GL_FRAMEBUFFER, index, GL_TEXTURE_CUBE_MAP_POSITIVE_X + layer, texture->id(), level);
        manager->intelCubemapDsaRebindFramebuffer();
    } else {
        glNamedFramebufferTextureLayer(id_, index, texture->id(), level, layer);
    }
}

void Framebuffer::attachRenderbuffer(int index, const Renderbuffer* renderbuffer) {
    renderbuffers_[mapAttachmentIndex(index)] = renderbuffer;
    if (index <= MAX_ATTACHMENTS) {
        index += GL_COLOR_ATTACHMENT0;
    }
    glNamedFramebufferRenderbuffer(id_, index, GL_RENDERBUFFER, renderbuffer->id());
}

const Texture* Framebuffer::getTexture(int index) const {
    return textures_[mapAttachmentIndex(index)];
}

const Renderbuffer* Framebuffer::getRenderbuffer(int index) const {
    return renderbuffers_[mapAttachmentIndex(index)];
}

}  // namespace GL
