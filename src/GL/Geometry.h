#pragma once

// #include <initializer_list>
#include <vector>

#include "Object.h"

namespace GL {

// [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glDrawElementsIndirect.xhtml#:~:text=packed%20into%20a%20structure)
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

// References:
// https://www.khronos.org/opengl/wiki/Buffer_Object
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

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBindBuffer.xhtml)
    void bind(GLenum target) const;

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBufferStorage.xhtml)
    void allocateEmpty(size_t size, GLbitfield flags);

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBufferData.xhtml)
    void allocateEmptyMutable(size_t size, GLenum usage);

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBufferStorage.xhtml)
    template <typename T>
    void allocate(const T* data, size_t size, GLbitfield flags) {
        allocate_(static_cast<const void*>(data), size, flags);
    }

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBufferData.xhtml)
    template <typename T>
    void allocateMutable(const T* data, size_t size, GLenum usage) {
        allocateMutable_(static_cast<const void*>(data), size, flags_);
    }

    /**
     * Grows the buffer to at least the spcified size.
     * It the buffer was allocated immutable it will get a new id and must be reattached to any vaos if it gorws.
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
};

// References:
// https://www.khronos.org/opengl/wiki/Vertex_Specification#Vertex_Array_Object
class VertexArray : public GLObject {
   private:
    GLuint id_ = 0;
    std::vector<std::pair<size_t, size_t>> bindingRanges_;
    std::vector<Buffer*> ownedBuffers_;

    ~VertexArray() {
        checkDestroyed(GL_VERTEX_ARRAY);
    }

   public:
    VertexArray();

    void destroy();

    GLuint id() const;

    void setDebugLabel(const std::string& label);

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

}  // namespace GL
