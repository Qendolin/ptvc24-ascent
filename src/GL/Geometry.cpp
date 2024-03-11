#include "Geometry.h"

#include "../Utils.h"
#include "StateManager.h"

namespace GL {

void warnAllocationSize(size_t size);
bool warnAllocationSizeZero(size_t size);

Buffer::Buffer() {
    glCreateBuffers(1, &id_);
}

void Buffer::destroy() {
    if (id_ != 0) {
        glDeleteBuffers(1, &id_);
        manager->unbindBuffer(id_);
        id_ = 0;
    }
    delete this;
}

GLuint Buffer::id() const {
    return id_;
}

size_t Buffer::size() const {
    return size_;
}

void Buffer::setDebugLabel(const std::string& label) {
    glObjectLabel(GL_BUFFER, id_, -1, label.c_str());
}

void Buffer::bind(GLenum target) const {
    manager->bindBuffer(target, id_);
}

void Buffer::allocateEmpty(size_t size, GLbitfield flags) {
    if (immutable_) {
        PANIC("VBO is immutable");
    }
    if (warnAllocationSizeZero(size)) {
        return;
    }
    warnAllocationSize(size);
    glNamedBufferStorage(id_, size, nullptr, flags);
    this->size_ = size;
    this->flags_ = flags;
    immutable_ = true;
}

void Buffer::allocateEmptyMutable(size_t size, GLenum usage) {
    if (immutable_) {
        PANIC("VBO is immutable");
    }
    if (warnAllocationSizeZero(size)) {
        return;
    }
    warnAllocationSize(size);
    glNamedBufferData(id_, size, nullptr, usage);
    this->flags_ = usage;
    this->size_ = size;
}

void Buffer::allocate(const void* data, size_t size, GLbitfield flags) {
    if (immutable_) {
        PANIC("VBO is immutable");
    }
    if (warnAllocationSizeZero(size)) {
        return;
    }
    warnAllocationSize(size);
    glNamedBufferStorage(id_, size, data, flags);
    this->size_ = size;
    this->flags_ = flags;
    immutable_ = true;
}

void Buffer::allocateMutable(const void* data, size_t size, GLbitfield usage) {
    if (immutable_) {
        PANIC("VBO is immutable");
    }
    warnAllocationSize(size);
    glNamedBufferData(id_, size, data, usage);
    this->flags_ = usage;
    this->size_ = size;
}

bool Buffer::grow(size_t size) {
    if (size < this->size_) {
        return false;
    }
    int newSize = this->size_;
    int doubleSize = newSize + newSize;
    if (size > doubleSize) {
        newSize = size;
    } else if (this->size_ < 16384) {
        newSize = doubleSize;
    } else {
        while (0 < newSize && newSize < size) {
            newSize += newSize / 4;
        }
        if (newSize <= 0) {
            newSize = size;
        }
    }

    if (immutable_) {
        GLuint newBufferId;
        glCreateBuffers(1, &newBufferId);
        glNamedBufferStorage(newBufferId, newSize, nullptr, flags_);
        glCopyNamedBufferSubData(id_, newBufferId, 0, 0, this->size_);
        glDeleteBuffers(1, &id_);
        id_ = newBufferId;
    } else {
        GLuint copyBufferId;
        glCreateBuffers(1, &copyBufferId);
        glNamedBufferStorage(copyBufferId, this->size_, nullptr, 0);
        glCopyNamedBufferSubData(id_, copyBufferId, 0, 0, this->size_);
        glNamedBufferData(id_, newSize, nullptr, flags_);
        glCopyNamedBufferSubData(copyBufferId, id_, 0, 0, this->size_);
        glDeleteBuffers(1, &copyBufferId);
    }
    this->size_ = newSize;
    return true;
}

void warnAllocationSize(size_t size) {
    int lowerLimit = 1024 * 4;
    int upperLimit = 1024 * 1000 * 1000;
    if (size < lowerLimit) {
        std::string msg = "Small buffer allocation: " + std::to_string(size) + " < " + std::to_string(lowerLimit) + " bytes";
        glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_PERFORMANCE, 1, GL_DEBUG_SEVERITY_NOTIFICATION, -1, msg.c_str());
    } else if (size > upperLimit) {
        std::string msg = "Large buffer allocation: " + std::to_string(size) + " > " + std::to_string(upperLimit) + " bytes";
        glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_PERFORMANCE, 1, GL_DEBUG_SEVERITY_NOTIFICATION, -1, msg.c_str());
    }
}

bool warnAllocationSizeZero(size_t size) {
    if (size != 0) {
        return false;
    }
    std::string msg = "Zero size buffer allocation";
    glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, 1, GL_DEBUG_SEVERITY_MEDIUM, -1, msg.c_str());
    return true;
}

VertexArray::VertexArray() : bindingRanges_(32, std::pair(0, 0)) {
    glCreateVertexArrays(1, &id_);
}

void VertexArray::destroy() {
    if (id_ != 0) {
        glDeleteVertexArrays(1, &id_);
        manager->unbindVertexArray(id_);
        id_ = 0;
    }
    delete this;
}

GLuint VertexArray::id() const {
    return id_;
}

void VertexArray::setDebugLabel(const std::string& label) {
    glObjectLabel(GL_VERTEX_ARRAY, id_, -1, label.c_str());
}

void VertexArray::bind() const {
    manager->bindVertexArray(id_);
}

void VertexArray::layout(int bufferIndex, int attributeIndex, int size, GLenum dataType, bool normalized, int offset) {
    glEnableVertexArrayAttrib(id_, attributeIndex);
    glVertexArrayAttribFormat(id_, attributeIndex, size, dataType, normalized, offset);
    glVertexArrayAttribBinding(id_, attributeIndex, bufferIndex);
}

void VertexArray::layoutI(int bufferIndex, int attributeIndex, int size, GLenum dataType, int offset) {
    glEnableVertexArrayAttrib(id_, attributeIndex);
    glVertexArrayAttribIFormat(id_, attributeIndex, size, dataType, offset);
    glVertexArrayAttribBinding(id_, attributeIndex, bufferIndex);
}

void VertexArray::bindBuffer(int bufferIndex, const Buffer& vbo, int offset, int stride) {
    bindingRanges_[bufferIndex] = std::make_pair(offset, stride);
    glVertexArrayVertexBuffer(id_, bufferIndex, vbo.id(), offset, stride);
}

void VertexArray::reBindBuffer(int bufferIndex, const Buffer& vbo) {
    const auto& range = bindingRanges_[bufferIndex];
    glVertexArrayVertexBuffer(id_, bufferIndex, vbo.id(), range.first, range.second);
}

void VertexArray::bindElementBuffer(const Buffer& ebo) {
    glVertexArrayElementBuffer(id_, ebo.id());
}

void VertexArray::attribDivisor(int bufferIndex, int divisor) {
    glVertexArrayBindingDivisor(id_, bufferIndex, divisor);
}

}  // namespace GL
