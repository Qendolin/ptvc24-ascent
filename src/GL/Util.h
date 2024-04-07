#pragma once

#include <string>

#define GLEW_STATIC
#include <GL/glew.h>

namespace GL {

std::string getObjectNamespaceString(GLenum type);

std::string getObjectLabel(GLenum type, GLuint id);

}  // namespace GL
