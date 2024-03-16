#pragma once

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Initializes GLFW
GLFWwindow* createOpenGLContext(bool enableCompatibilityProfile);

// Initializes OpenGL using GLEW.
// Sets the debug callback.
void initializeOpenGL(bool enableDebug);