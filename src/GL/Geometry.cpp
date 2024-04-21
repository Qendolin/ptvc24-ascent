#include "Geometry.h"

#include "../Util/Log.h"
#include "StateManager.h"

namespace gl {

void warnAllocationSize(size_t size);
bool warnAllocationSizeZero(size_t size);

Buffer::Buffer() : GLObject(GL_BUFFER) {
    glCreateBuffers(1, &id_);
    track_();
}

Buffer::~Buffer() {
    if (id_ != 0) {
        if (isMapped_) {
            unmap();
        }
        glDeleteBuffers(1, &id_);
        manager->unbindBuffer(id_);
        untrack_();
        id_ = 0;
    }
}

size_t Buffer::size() const {
    return size_;
}

void Buffer::bind(GLenum target) const {
    manager->bindBuffer(target, id_);
}

void Buffer::allocateEmpty(size_t size, GLbitfield flags) {
    if (warnAllocationSizeZero(size)) {
        return;
    }
    warnAllocationSize(size);
    glNamedBufferStorage(id_, size, nullptr, flags);
    this->size_ = size;
    this->flags_ = flags;
}

void Buffer::allocate_(const void* data, size_t size, GLbitfield flags) {
    if (warnAllocationSizeZero(size)) {
        return;
    }
    warnAllocationSize(size);
    glNamedBufferStorage(id_, size, data, flags);
    this->size_ = size;
    this->flags_ = flags;
}

bool Buffer::grow(size_t size) {
    if (size < this->size_) {
        return false;
    }
    size_t new_size = this->size_;
    size_t double_size = new_size + new_size;
    // try to be smart about growing the buffer
    if (size > double_size) {
        new_size = size;
    } else if (this->size_ < 16384) {
        new_size = double_size;
    } else {
        while (0 < new_size && new_size < size) {
            new_size += new_size / 4;
        }
        if (new_size <= 0) {
            new_size = size;
        }
    }

    GLuint new_buffer_id;
    glCreateBuffers(1, &new_buffer_id);
    glNamedBufferStorage(new_buffer_id, new_size, nullptr, flags_);
    glCopyNamedBufferSubData(id_, new_buffer_id, 0, 0, this->size_);
    glDeleteBuffers(1, &id_);
    untrack_();
    id_ = new_buffer_id;
    track_();

    // carry over the debug label
    if (!debugLabel().empty()) setDebugLabel(debugLabel());
    this->size_ = new_size;
    return true;
}

void warnAllocationSize(size_t size) {
#ifndef NDEBUG
    int lower_limit = 1024 * 4;
    int upper_limit = 1024 * 1000 * 1000;
    if (size < lower_limit) {
        std::string msg = "Small buffer allocation: " + std::to_string(size) + " < " + std::to_string(lower_limit) + " bytes";
        glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_PERFORMANCE, 1, GL_DEBUG_SEVERITY_NOTIFICATION, -1, msg.c_str());
    } else if (size > upper_limit) {
        std::string msg = "Large buffer allocation: " + std::to_string(size) + " > " + std::to_string(upper_limit) + " bytes";
        glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_PERFORMANCE, 1, GL_DEBUG_SEVERITY_NOTIFICATION, -1, msg.c_str());
    }
#endif
}

bool warnAllocationSizeZero(size_t size) {
    if (size != 0) {
        return false;
    }
#ifndef NDEBUG
    std::string msg = "Zero size buffer allocation";
    glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, 1, GL_DEBUG_SEVERITY_MEDIUM, -1, msg.c_str());
#endif
    return true;
}

VertexArray::VertexArray() : GLObject(GL_VERTEX_ARRAY), bindingRanges_(32, std::pair(0, 0)) {
    glCreateVertexArrays(1, &id_);
    track_();
}

VertexArray::~VertexArray() {
    if (id_ != 0) {
        glDeleteVertexArrays(1, &id_);
        manager->unbindVertexArray(id_);
        untrack_();
        id_ = 0;
    }

    for (auto&& buf : ownedBuffers_) {
        delete buf;
    }
}

void VertexArray::bind() const {
    manager->bindVertexArray(id_);
}

void VertexArray::layout(int buffer_index, int attribute_index, int size, GLenum data_type, bool normalized, size_t offset) {
    glEnableVertexArrayAttrib(id_, attribute_index);
    glVertexArrayAttribFormat(id_, attribute_index, size, data_type, normalized, offset);
    glVertexArrayAttribBinding(id_, attribute_index, buffer_index);
}

void VertexArray::layoutI(int buffer_index, int attribute_index, int size, GLenum data_type, size_t offset) {
    glEnableVertexArrayAttrib(id_, attribute_index);
    glVertexArrayAttribIFormat(id_, attribute_index, size, data_type, offset);
    glVertexArrayAttribBinding(id_, attribute_index, buffer_index);
}

void VertexArray::bindBuffer(int buffer_index, const Buffer& vbo, size_t offset, size_t stride) {
    bindingRanges_[buffer_index] = std::make_pair(offset, stride);
    glVertexArrayVertexBuffer(id_, buffer_index, vbo.id(), offset, stride);
}

void VertexArray::reBindBuffer(int buffer_index, const Buffer& vbo) {
    const auto& range = bindingRanges_[buffer_index];
    glVertexArrayVertexBuffer(id_, buffer_index, vbo.id(), range.first, range.second);
}

void VertexArray::bindElementBuffer(const Buffer& ebo) {
    glVertexArrayElementBuffer(id_, ebo.id());
}

void VertexArray::attribDivisor(int buffer_index, int divisor) {
    glVertexArrayBindingDivisor(id_, buffer_index, divisor);
}

void VertexArray::own(const std::initializer_list<Buffer*> buffers) {
    for (auto& buffer : buffers) {
        own(buffer);
    }
}

void VertexArray::own(Buffer* buffer) {
    ownedBuffers_.push_back(buffer);
}

}  // namespace gl
