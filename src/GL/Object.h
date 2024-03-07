#pragma once
#define GLEW_STATIC
#include <GL/glew.h>

#include <iostream>
#include <string>

namespace GL {

class GLObject {
   protected:
    void checkDestroyed(GLenum type) {
        GLuint id = this->id();
        if (id == 0) return;

        GLsizei len;
        char label[256];
        glGetObjectLabel(type, id, sizeof(label), &len, &label[0]);

        std::string labelString = "";
        if (len != 0) {
            labelString = " label='" + std::string(label, label + len) + "'";
        }

        const char* typeString;
        switch (type) {
            case GL_BUFFER:
                typeString = "Buffer";
                break;
            case GL_SHADER:
                typeString = "Shader";
                break;
            case GL_PROGRAM:
                typeString = "Shader Program";
                break;
            case GL_VERTEX_ARRAY:
                typeString = "Vertex Array";
                break;
            case GL_PROGRAM_PIPELINE:
                typeString = "Program Pipeline";
                break;
            case GL_SAMPLER:
                typeString = "Sampler";
                break;
            case GL_TEXTURE:
                typeString = "Texture";
                break;
            case GL_RENDERBUFFER:
                typeString = "Renderbuffer";
                break;
            case GL_FRAMEBUFFER:
                typeString = "Framebuffer";
                break;
            default:
                typeString = "Unknwon";
                break;
        }
        std::cerr << "GL " << typeString << " Object id=" << std::to_string(id) << labelString << " not destroyed!" << std::endl;
    }

   public:
    virtual void destroy() = 0;
    virtual GLuint id() const = 0;
    virtual void setDebugLabel(const std::string& label) = 0;

    // prevent copy
    GLObject(GLObject const&) = delete;
    GLObject& operator=(GLObject const&) = delete;
    GLObject() {}

    virtual ~GLObject() {}
};

}  // namespace GL
