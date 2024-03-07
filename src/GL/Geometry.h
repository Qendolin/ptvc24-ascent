#pragma once
#include <vector>

#include "Object.h"

namespace GL {

class Buffer : public GLObject {
   private:
    GLuint id_ = 0;
    size_t size_ = 0;
    uint32_t flags_ = 0;
    bool immutable_ = false;

    void allocate(const void* data, size_t size, GLbitfield flags);
    void allocateMutable(const void* data, size_t size, GLenum usage);

   public:
    Buffer();

    ~Buffer() {
        checkDestroyed(GL_BUFFER);
    }

    void destroy();

    GLuint id() const;

    size_t size() const;

    void setDebugLabel(const std::string& label);

    void bind(GLenum target) const;

    void allocateEmpty(size_t size, GLbitfield flags);

    void allocateEmptyMutable(size_t size, GLenum usage);

    template <typename T>
    void allocate(const T* data, size_t size, GLbitfield flags) {
        allocate(static_cast<const void*>(data), size, flags);
    }

    template <typename T>
    void allocateMutable(const T* data, size_t size, GLenum usage) {
        allocateMutable(static_cast<const void*>(data), size, flags_);
    }

    bool grow(size_t size);

    template <typename T>
    void write(int offset, const T* data, size_t size) {
        glNamedBufferSubData(id_, offset, size, data);
    }

    template <typename T>
    void writeIndex(int index, const T data) {
        size_t size = sizeof(data);
        glNamedBufferSubData(id_, index * size, size, data);
    }
};

class VertexArray : public GLObject {
   private:
    GLuint id_ = 0;
    std::vector<std::pair<int, int>> bindingRanges_;

   public:
    VertexArray();

    ~VertexArray() {
        checkDestroyed(GL_VERTEX_ARRAY);
    }

    void destroy();

    GLuint id() const;

    void setDebugLabel(const std::string& label);

    void bind() const;

    void layout(int bufferIndex, int attributeIndex, size_t size, GLenum dataType, bool normalized, int offset);

    void layoutI(int bufferIndex, int attributeIndex, size_t size, GLenum dataType, int offset);

    void bindBuffer(int bufferIndex, const Buffer& vbo, int offset, int stride);

    void reBindBuffer(int bufferIndex, const Buffer& vbo);

    void bindElementBuffer(const Buffer& ebo);

    void attribDivisor(int bufferIndex, int divisor);
};

}  // namespace GL
