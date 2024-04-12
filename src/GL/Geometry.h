#pragma once

#include <vector>

#include "Object.h"

namespace gl {

// References:
// https://www.khronos.org/opengl/wiki/Buffer_Object
class Buffer : public GLObject {
   private:
    size_t size_ = 0;
    uint32_t flags_ = 0;
    bool isMapped_ = false;

    void allocate_(const void* data, size_t size, GLbitfield flags);

   public:
    Buffer();
    virtual ~Buffer();

    Buffer(Buffer&&) noexcept = default;

    // @returns allocated buffer size in bytes
    size_t size() const;

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBindBuffer.xhtml)
    void bind(GLenum target) const;

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBufferStorage.xhtml)
    void allocateEmpty(size_t size, GLbitfield flags);

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBufferStorage.xhtml)
    template <typename T>
    void allocate(const T* data, size_t size, GLbitfield flags) {
        allocate_(static_cast<const void*>(data), size, flags);
    }

    /**
     * Grows the buffer to at least the spcified size.
     * The must be reattached to any vaos if it gorws!
     * @param size size in bytes
     * @returns `true` if the buffer did grow.
     */
    bool grow(size_t size);

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBufferSubData.xhtml)
    template <typename T>
    void write(size_t offset, const T* data, size_t size) {
        glNamedBufferSubData(id_, offset, size, data);
    }

    // Better not use this
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBufferSubData.xhtml)
    template <typename T>
    void writeIndex(int index, const T data) {
        size_t size = sizeof(data);
        glNamedBufferSubData(id_, index * size, size, data);
    }

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glMapBufferRange.xhtml)
    template <typename T>
    T* mapRange(GLbitfield flags) {
        isMapped_ = true;
        return reinterpret_cast<T*>(glMapNamedBufferRange(id_, 0, size_, flags));
    }

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glMapBufferRange.xhtml)
    template <typename T>
    T* mapRange(size_t offset, size_t length, GLbitfield flags) {
        isMapped_ = true;
        return reinterpret_cast<T*>(glMapNamedBufferRange(id_, offset, length, flags));
    }

    void unmap() {
        glUnmapNamedBuffer(id_);
        isMapped_ = false;
    }
};

// References:
// https://www.khronos.org/opengl/wiki/Vertex_Specification#Vertex_Array_Object
class VertexArray : public GLObject {
   private:
    std::vector<std::pair<size_t, size_t>> bindingRanges_;
    std::vector<Buffer*> ownedBuffers_;

   public:
    VertexArray();
    virtual ~VertexArray();

    VertexArray(VertexArray&&) noexcept = default;

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBindVertexArray.xhtml)
    void bind() const;

    // Can't include the references in the doc comment because the layout is broken.
    // References:
    // - [Wiki](https://www.khronos.org/opengl/wiki/Vertex_Specification#Separate_attribute_format)
    // - [glVertexAttribFormat](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glVertexAttribFormat.xhtml)
    // - [glVertexAttribBinding](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glVertexAttribBinding.xhtml)
    // - [glEnableVertexAttribArray](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glEnableVertexAttribArray.xhtml)
    /**
     * Enable the attribute and specify its layout. (See comment above for refereces)
     *
     * @param buffer_index a buffer binding index. See `bindBuffer`.
     * @param attribute_index a attribute index. Used by the shader. [Reference](https://www.khronos.org/opengl/wiki/Layout_Qualifier_(GLSL)#Vertex_shader_attribute_index)
     * @param size the number of components. `1, 2, 3, 4` for `float, vec2, vec3, vec4`.
     * @param data_type the data type. Usually `GL_FLOAT`.
     * @param normalized normalizes integer data to the range `[-1,1]` or `[0,1]` if it is signed or unsiged.
     * @param offset an offset, in bytes, into the vertex buffer, where the data starts.
     */
    void layout(int buffer_index, int attribute_index, int size, GLenum data_type, bool normalized, size_t offset);

    // Same as `layout` but for integer formats
    void layoutI(int buffer_index, int attribute_index, int size, GLenum data_type, size_t offset);

    /**
     * [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBindVertexBuffer.xhtml)
     *
     * @param buffer_index a buffer binding index. See `layout`
     * @param vbo the buffer to bind to the binding index
     * @param offset an offset, in bytes, into the buffer, where the data starts.
     * @param stride the number of bytes between buffer elements. For tighly packed elements it is the size of the element itself.
     */
    void bindBuffer(int buffer_index, const Buffer& vbo, size_t offset, size_t stride);

    // binds the buffer to the given index using the offset and stride that it had previously
    void reBindBuffer(int buffer_index, const Buffer& vbo);

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glVertexArrayElementBuffer.xhtml)
    void bindElementBuffer(const Buffer& ebo);

    // References:
    // [Wiki](https://www.khronos.org/opengl/wiki/Vertex_Specification#Instanced_arrays)
    // [glVertexBindingDivisor](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glVertexBindingDivisor.xhtml)
    void attribDivisor(int buffer_index, int divisor);

    // Give ownership of the given buffers, meaning they will be destroyed with the vertex array
    void own(const std::initializer_list<Buffer*> buffers);

    // Give ownership of the given buffer, meaning it will be destroyed with the vertex array
    void own(Buffer* buffer);
};

}  // namespace gl
