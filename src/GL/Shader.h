#pragma once
#include <glm/glm.hpp>
#include <map>
#include <string>
#include <vector>

#include "Object.h"

namespace GL {

// References:
// https://www.khronos.org/opengl/wiki/Shader
// https://www.khronos.org/opengl/wiki/GLSL_Object
class ShaderProgram : public GLObject {
   private:
    GLuint id_ = 0;
    std::map<std::string, int32_t> uniformLocations_;
    std::string sourceOriginal_ = "";
    std::string sourceModified_ = "";
    GLenum stage_ = 0;

    ~ShaderProgram() {
        checkDestroyed(GL_PROGRAM);
    }

   public:
    // Load and compile the given source code.
    // See `compile` for details.
    ShaderProgram(std::string source, GLenum stage, std::map<std::string, std::string> substitutions = {});
    // Load and compile the given shader file. The stage is determined by the file extension.
    // See `compile` for details.
    ShaderProgram(std::string filename, std::map<std::string, std::string> substitutions = {});

    void destroy();

    std::string source() const;

    GLuint id() const;

    GLenum stage() const;

    // Compiles the shader source code with the provided string substituions.
    // Useful to set certain values or enable code at compile time.
    // References:
    // - https://registry.khronos.org/OpenGL-Refpages/gl4/html/glCreateShaderProgram.xhtml
    // - https://registry.khronos.org/OpenGL-Refpages/gl4/html/glValidateProgram.xhtml
    void compile(std::map<std::string, std::string> substitutions = {});

    void setDebugLabel(const std::string& label);

    // Get the uniform location index given its name. Uses caching.
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGetUniformLocation.xhtml)
    GLint getUniformLocation(const std::string& name);

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glUniform.xhtml)
    template <typename T>
    void setUniform(const std::string& name, T value);

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glUniform.xhtml)
    template <typename T>
    void setUniformIndexed(const std::string& name, int index, T value);
};

// References:
// https://www.khronos.org/opengl/wiki/Shader_Compilation#Separate_programs
class ShaderPipeline : public GLObject {
   private:
    GLuint id_ = 0;
    ShaderProgram* vertStage_ = nullptr;
    ShaderProgram* tessCtrlStage_ = nullptr;
    ShaderProgram* tessEvalStage_ = nullptr;
    ShaderProgram* geomStage_ = nullptr;
    ShaderProgram* fragStage_ = nullptr;
    ShaderProgram* compStage_ = nullptr;
    std::vector<ShaderProgram*> ownedPrograms_;

    ~ShaderPipeline() {
        checkDestroyed(GL_PROGRAM_PIPELINE);
        for (auto&& p : ownedPrograms_) {
            p->destroy();
        }
        ownedPrograms_ = {};
    }

    ShaderProgram*& getRef_(GLenum stage);

   public:
    ShaderPipeline();
    // Initialize a pipeline with owned programs. See `own` for details.
    // They will be automatically attached.
    ShaderPipeline(std::initializer_list<ShaderProgram*> owned_programs);

    void destroy();

    void bind() const;

    GLuint id() const;

    // @returns the vertex shader
    ShaderProgram* vertexStage() const;

    // @returns the fragment shader
    ShaderProgram* fragmentStage() const;

    // @returns the shader for the given stage
    ShaderProgram* get(GLenum stage) const;

    void setDebugLabel(const std::string& label);

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glUseProgramStages.xhtml)
    void attach(const std::initializer_list<ShaderProgram*> programs);

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glUseProgramStages.xhtml)
    void attach(ShaderProgram* program);

    // Give ownership of the given programs, meaning they will be destroyed with the pipeline
    void own(const std::initializer_list<ShaderProgram*> programs);

    // Give ownership of the given program, meaning it will be destroyed with the pipeline
    void own(ShaderProgram* program);

    void reAttach(GLenum stage);

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glUseProgramStages.xhtml)
    void detach(GLenum stage);
};
}  // namespace GL
