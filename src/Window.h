#pragma once

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

struct GLFWwindow;

struct Window {
    GLFWwindow *handle = nullptr;

    // The size of the content area / viewport
    struct {
        int x = 1600;
        int y = 900;
    } size;

    Window(GLFWwindow *handle) : handle(handle){};

    // Implicit conversion constructor for convenience
    operator GLFWwindow *() const {
        return handle;
    }
};