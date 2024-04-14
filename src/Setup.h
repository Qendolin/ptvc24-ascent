#pragma once

struct Window;

// Initializes GLFW
Window createOpenGLContext(bool enableCompatibilityProfile);

// Initializes OpenGL using GLEW.
// Sets the debug callback.
void initializeOpenGL(bool enableDebug);

void destroyOpenGLContext(Window &window);