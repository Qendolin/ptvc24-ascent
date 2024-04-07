#include "Util.h"

namespace GL {

std::string getObjectNamespaceString(GLenum type) {
    switch (type) {
        case GL_BUFFER:
            return "Buffer";
        case GL_SHADER:
            return "Shader";
        case GL_PROGRAM:
            return "Shader Program";
        case GL_VERTEX_ARRAY:
            return "Vertex Array";
        case GL_PROGRAM_PIPELINE:
            return "Program Pipeline";
        case GL_SAMPLER:
            return "Sampler";
        case GL_TEXTURE:
            return "Texture";
        case GL_RENDERBUFFER:
            return "Renderbuffer";
        case GL_FRAMEBUFFER:
            return "Framebuffer";
        default:
            return "Unknwon";
    }
}

std::string getObjectLabel(GLenum type, GLuint id) {
    GLsizei len;
    char label[256];
    glGetObjectLabel(type, id, sizeof(label), &len, &label[0]);
    return std::string(label, label + len);
}

}  // namespace GL
