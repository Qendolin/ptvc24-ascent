#pragma once
#define GLEW_STATIC
#include <GL/glew.h>

#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <sstream>
#include <stacktrace>
#include <string>

#define LOG(msg) \
    std::cout << __FILE__ << "(" << __LINE__ << "): " << msg << std::endl

#define PANIC(msg) \
    throw std::runtime_error(msg + std::string("\n") + std::to_string(std::stacktrace::current()));

static std::string formatDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity, const char* msg) {
    std::stringstream stringStream;
    std::string sourceString;
    std::string typeString;
    std::string severityString;

    // The AMD variant of this extension provides a less detailed classification of the error,
    // which is why some arguments might be "Unknown".
    switch (source) {
        case GL_DEBUG_CATEGORY_API_ERROR_AMD:
        case GL_DEBUG_SOURCE_API: {
            sourceString = "API";
            break;
        }
        case GL_DEBUG_CATEGORY_APPLICATION_AMD:
        case GL_DEBUG_SOURCE_APPLICATION: {
            sourceString = "Application";
            break;
        }
        case GL_DEBUG_CATEGORY_WINDOW_SYSTEM_AMD:
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: {
            sourceString = "Window System";
            break;
        }
        case GL_DEBUG_CATEGORY_SHADER_COMPILER_AMD:
        case GL_DEBUG_SOURCE_SHADER_COMPILER: {
            sourceString = "Shader Compiler";
            break;
        }
        case GL_DEBUG_SOURCE_THIRD_PARTY: {
            sourceString = "Third Party";
            break;
        }
        case GL_DEBUG_CATEGORY_OTHER_AMD:
        case GL_DEBUG_SOURCE_OTHER: {
            sourceString = "Other";
            break;
        }
        default: {
            sourceString = "Unknown";
            break;
        }
    }

    switch (type) {
        case GL_DEBUG_TYPE_ERROR: {
            typeString = "Error";
            break;
        }
        case GL_DEBUG_CATEGORY_DEPRECATION_AMD:
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: {
            typeString = "Deprecated Behavior";
            break;
        }
        case GL_DEBUG_CATEGORY_UNDEFINED_BEHAVIOR_AMD:
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: {
            typeString = "Undefined Behavior";
            break;
        }
        case GL_DEBUG_TYPE_PORTABILITY_ARB: {
            typeString = "Portability";
            break;
        }
        case GL_DEBUG_CATEGORY_PERFORMANCE_AMD:
        case GL_DEBUG_TYPE_PERFORMANCE: {
            typeString = "Performance";
            break;
        }
        case GL_DEBUG_CATEGORY_OTHER_AMD:
        case GL_DEBUG_TYPE_OTHER: {
            typeString = "Other";
            break;
        }
        default: {
            typeString = "Unknown";
            break;
        }
    }

    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH: {
            severityString = "High";
            break;
        }
        case GL_DEBUG_SEVERITY_MEDIUM: {
            severityString = "Medium";
            break;
        }
        case GL_DEBUG_SEVERITY_LOW: {
            severityString = "Low";
            break;
        }
        case GL_DEBUG_SEVERITY_NOTIFICATION: {
            severityString = "Info";
            break;
        }
        default: {
            severityString = "Unknown";
            break;
        }
    }

    stringStream << "OpenGL Message: " << msg;
    stringStream << " [Source = " << sourceString;
    stringStream << ", Type = " << typeString;
    stringStream << ", Severity = " << severityString;
    stringStream << ", ID = " << id << "]";

    return stringStream.str();
}

static std::string loadFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        PANIC("Error opening file: " + filename);
    }

    // Read the entire file into a string
    std::string content;
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    content.resize(size);
    file.seekg(0, std::ios::beg);
    file.read(content.data(), size);

    return content;
}