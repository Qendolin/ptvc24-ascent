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

    void checkDestroyed();

    virtual ~GLObject() {
        checkDestroyed();
    }

    void track_();

    void untrack_();

   public:
    // prevent copy
    GLObject(GLObject const&) = delete;
    GLObject& operator=(GLObject const&) = delete;

    // allow move
    GLObject(GLObject&& other) noexcept : type_(other.type_), id_(std::exchange(other.id_, 0)) {}

    virtual void destroy() = 0;
    virtual void setDebugLabel(const std::string& label) = 0;

    GLuint id() const {
        return id_;
    };
};

}  // namespace GL
