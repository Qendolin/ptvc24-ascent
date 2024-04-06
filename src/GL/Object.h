#pragma once
#define GLEW_STATIC
#include <GL/glew.h>

#include <iostream>
#include <string>

namespace GL {

class GLObject {
   protected:
    GLenum type_ = GL_NONE;
    GLuint id_ = 0;

    /**
     * @param type One of the namespaces in the table [here](https://www.khronos.org/opengl/wiki/OpenGL_Object#Object_names).
     */
    GLObject(GLenum type) : type_(type) {}

    void checkDestroyed() {
        if (id_ == 0) return;

        GLsizei len;
        char label[256];
        glGetObjectLabel(type_, id_, sizeof(label), &len, &label[0]);

        std::string labelString = "";
        if (len != 0) {
            labelString = " label='" + std::string(label, label + len) + "'";
        }

        const char* typeString;
        switch (type_) {
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
        std::cerr << "GL " << typeString << " Object id=" << std::to_string(id_) << labelString << " not destroyed!" << std::endl;
    }

    ~GLObject() {
        checkDestroyed();
    }

   public:
    // prevent copy
    GLObject(GLObject const&) = delete;
    GLObject& operator=(GLObject const&) = delete;

    // allow move
    GLObject(GLObject&& other) noexcept : type_(other.type_), id_(std::exchange(other.id_, 0)) {
        std::cout << "!!! Moved " << id_ << std::endl;
    }

    virtual void destroy() = 0;
    virtual void setDebugLabel(const std::string& label) = 0;

    GLuint id() const {
        return id_;
    };
};

}  // namespace GL
