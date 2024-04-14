#pragma once

#define GLEW_STATIC
#include <GL/glew.h>

#include <string>

namespace gl {

std::string getObjectNamespaceString(GLenum type);

std::string getObjectLabel(GLenum type, GLuint id);

}  // namespace gl
