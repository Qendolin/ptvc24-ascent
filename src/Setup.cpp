#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <exception>
#include <memory>
#include <string>

#include "GL/StateManager.h"
#include "Utils.h"

static std::string formatDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity, const char *msg);

static void APIENTRY debugCallback(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar *message,
    const GLvoid *userParam) {
    if (id == 131185 || id == 131218) {
        // glDebugMessageControl is ignored in nsight, so double check to prevent log spam
        return;
    }

    auto group_stack = *reinterpret_cast<const std::vector<std::string> *>(userParam);

    if (type == GL_DEBUG_TYPE_PUSH_GROUP) {
        group_stack.push_back(message);
        return;
    } else if (type == GL_DEBUG_TYPE_POP_GROUP) {
        group_stack.pop_back();
        return;
    }

    std::string severity_str;
    std::string type_str;
    std::string source_str;

    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            severity_str = "CRITICAL_ERROR";
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            severity_str = "ERROR";
            break;
        case GL_DEBUG_SEVERITY_LOW:
            severity_str = "WARNING";
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            severity_str = "INFO";
            break;
    }

    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            type_str = "ERROR";
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            type_str = "DEPRECATED_BEHAVIOR";
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            type_str = "UNDEFINED_BEHAVIOR";
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            type_str = "PERFORMANCE";
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            type_str = "PORTABILITY";
            break;
        case GL_DEBUG_TYPE_OTHER:
            type_str = "OTHER";
            break;
        case GL_DEBUG_TYPE_MARKER:
            type_str = "MARKER";
            break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
            type_str = "PUSH_GROUP";
            break;
        case GL_DEBUG_TYPE_POP_GROUP:
            type_str = "POP_GROUP";
            break;
    }

    switch (source) {
        case GL_DEBUG_SOURCE_API:
            source_str = "GRAPHICS_LIBRARY";
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            source_str = "SHADER_COMPILER";
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            source_str = "WINDOW_SYSTEM";
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            source_str = "THIRD_PARTY";
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            source_str = "APPLICATION";
            break;
        case GL_DEBUG_SOURCE_OTHER:
            source_str = "OTHER";
            break;
    }

    std::string err = "[" + severity_str + "] " + type_str + " #" + std::to_string(id) + " from " + source_str + ": " + message;

    if (severity == GL_DEBUG_SEVERITY_HIGH) {
        std::string stack;
        for (const auto &group : group_stack) {
            stack += group + " > ";
        }
        stack = stack.substr(0, stack.length() - 3);
        throw std::runtime_error(err + "\nDebug group: " + stack + "\nTrace: " + std::to_string(std::stacktrace::current()));
    }

    std::cout << err << std::endl;
}

void setupOpenGL(bool enableCompatibilityProfile, bool disableGlDebug) {
    LOG("Initializing GLFW");
    if (glfwInit() != GLFW_TRUE) {
        throw std::runtime_error("GLFW init failed");
    }

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    if (enableCompatibilityProfile) {
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
        LOG("Using GL compatability profile");
    } else {
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        LOG("Using GL core profile");
    }

    LOG("Creating Window");
    GLFWwindow *win = glfwCreateWindow(1600, 900, "Ascent", nullptr, nullptr);
    if (win == nullptr) {
        throw std::runtime_error("GLFW window creation failed");
    }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(0);

    LOG("Initializing OpenGL");

    glewExperimental = true;
    GLenum err = glewInit();

    // If GLEW wasn't initialized
    if (err != GLEW_OK) {
        throw std::runtime_error(std::format(
            "Glew init failed: {}",
            reinterpret_cast<const char *>(glewGetErrorString(err))));
    }

    LOG("Using GPU: " << glGetString(GL_RENDERER));

    GL::manager = std::make_unique<GL::StateManager>(GL::createEnvironment());

    // enable these without using the manager
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    if (glDebugMessageCallback != NULL && !disableGlDebug) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

        std::vector<std::string> *group_stack = new std::vector<std::string>{"top"};
        glDebugMessageCallback(debugCallback, group_stack);
        glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_PUSH_GROUP, GL_DONT_CARE, 0, nullptr, false);
        glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_POP_GROUP, GL_DONT_CARE, 0, nullptr, false);
        const GLuint disabledMessages[] = {131185, 131218};
        glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_OTHER, GL_DONT_CARE, 2, &disabledMessages[0], false);
    }
}