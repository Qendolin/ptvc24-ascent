#pragma once

// #include <initializer_list>
#include <vector>

#include "Object.h"

namespace GL {

// See: https://registry.khronos.org/OpenGL-Refpages/gl4/html/glDrawElementsIndirect.xhtml#:~:text=packed%20into%20a%20structure
typedef struct DrawElementsIndirectCommand {
    // The element count
    uint32_t count;
    uint32_t instanceCount;
    // The offset for the first index of the mesh in the ebo
    uint32_t firstIndex;
    // The offset for the first vertex of the mesh in the vbo
    int32_t baseVertex;
    uint32_t baseInstance;
} DrawElementsIndirectCommand;

class Buffer : public GLObject {
   private:
    GLuint id_ = 0;
    size_t size_ = 0;
    uint32_t flags_ = 0;
    bool immutable_ = false;

    void allocate_(const void* data, size_t size, GLbitfield flags);
    void allocateMutable_(const void* data, size_t size, GLenum usage);

    ~Buffer() {
        checkDestroyed(GL_BUFFER);
    }

   public:
    Buffer();

    void destroy();

    GLuint id() const;

    size_t size() const;

    void setDebugLabel(const std::string& label);

    void bind(GLenum target) const;

    void allocateEmpty(size_t size, GLbitfield flags);

    void allocateEmptyMutable(size_t size, GLenum usage);

    template <typename T>
    void allocate(const T* data, size_t size, GLbitfield flags) {
        allocate_(static_cast<const void*>(data), size, flags);
    }

    template <typename T>
    void allocateMutable(const T* data, size_t size, GLenum usage) {
        allocateMutable_(static_cast<const void*>(data), size, flags_);
    }

    bool grow(size_t size);

    template <typename T>
    void write(size_t offset, const T* data, size_t size) {
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
    std::vector<Buffer*> ownedBuffers_;

    ~VertexArray() {
        checkDestroyed(GL_VERTEX_ARRAY);
    }

   public:
    VertexArray();

    void destroy();

    GLuint id() const;

    void setDebugLabel(const std::string& label);

    void bind() const;

    void layout(int bufferIndex, int attributeIndex, int size, GLenum dataType, bool normalized, int offset);

    void layoutI(int bufferIndex, int attributeIndex, int size, GLenum dataType, int offset);

    void bindBuffer(int bufferIndex, const Buffer& vbo, int offset, int stride);

    void reBindBuffer(int bufferIndex, const Buffer& vbo);

    void bindElementBuffer(const Buffer& ebo);

    void attribDivisor(int bufferIndex, int divisor);

    void own(const std::initializer_list<Buffer*> buffers);

    void own(Buffer* buffer);
};

}  // namespace GL
