#pragma once

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

struct GLFWwindow;

struct Window {
    GLFWwindow *handle = nullptr;

    // The size is the real window size minus the border and title bar
    glm::ivec2 size = {1600, 900};

    Window(GLFWwindow *handle) : handle(handle){};

    // Implicit conversion constructor for convenience
    operator GLFWwindow *() const {
        return handle;
    }
};