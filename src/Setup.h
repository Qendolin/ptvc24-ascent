#pragma once

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

GLFWwindow* createOpenGLContext(bool enableCompatibilityProfile);
void initializeOpenGL(bool enableDebug);