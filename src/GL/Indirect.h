#pragma once

namespace gl {
// [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glDrawElementsIndirect.xhtml#:~:text=packed%20into%20a%20structure)
struct DrawElementsIndirectCommand {
    // The element count
    uint32_t count;
    uint32_t instanceCount;
    // The offset for the first index of the mesh in the ebo
    uint32_t firstIndex;
    // The offset for the first vertex of the mesh in the vbo
    int32_t baseVertex;
    uint32_t baseInstance;
};

}  // namespace gl
