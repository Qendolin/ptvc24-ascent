#pragma once

#define GLEW_STATIC
#define GLEW_NO_GLU
#include <GL/glew.h>

#include <iostream>
#include <string>

namespace gl {

class GLObject {
   protected:
    GLenum type_ = GL_NONE;
    GLuint id_ = 0;
    std::string debugLabel_ = "";

    /**
     * @param type One of the namespaces in the table [here](https://www.khronos.org/opengl/wiki/OpenGL_Object#Object_names).
     */
    GLObject(GLenum type) : type_(type) {}

    void track_();

    void untrack_();

   public:
    // prevent copy
    GLObject(GLObject const&) = delete;
    GLObject& operator=(GLObject const&) = delete;

    // allow move
    GLObject(GLObject&& other);

    virtual ~GLObject();

    virtual void setDebugLabel(const std::string& label);

    std::string debugLabel() {
        return debugLabel_;
    }

    GLuint id() const {
        return id_;
    };
};

}  // namespace gl
