#pragma once
#include <glm/glm.hpp>
#include <map>
#include <string>

#include "Object.h"

namespace GL {

class ShaderProgram : public GLObject {
   private:
    GLuint id_ = 0;
    std::map<std::string, int32_t> uniformLocations_;
    std::string sourceOriginal_ = "";
    std::string sourceModified_ = "";
    GLenum stage_ = 0;

   public:
    ShaderProgram(std::string source, GLenum stage);

    ~ShaderProgram() {
        checkDestroyed(GL_PROGRAM);
    }

    void destroy();

    std::string source() const;

    GLuint id() const;

    GLenum stage() const;

    void compile();

    void compileWith(std::map<std::string, std::string> defs);

    void setDebugLabel(const std::string& label);

    GLint getUniformLocation(const std::string& name);

    template <typename T>
    void setUniform(const std::string& name, T value);

    template <typename T>
    void setUniformIndexed(const std::string& name, int index, T value);
};

class ShaderPipeline : public GLObject {
   private:
    GLuint id_ = 0;
    ShaderProgram* vertStage_ = nullptr;
    ShaderProgram* tessCtrlStage_ = nullptr;
    ShaderProgram* tessEvalStage_ = nullptr;
    ShaderProgram* geomStage_ = nullptr;
    ShaderProgram* fragStage_ = nullptr;
    ShaderProgram* compStage_ = nullptr;

   public:
    ShaderPipeline();

    ~ShaderPipeline() {
        checkDestroyed(GL_PROGRAM_PIPELINE);
    }

    void destroy();

    void bind() const;

    GLuint id() const;

    ShaderProgram* vertexStage() const;

    ShaderProgram* fragmentStage() const;

    ShaderProgram* get(GLenum stage) const;

    void setDebugLabel(const std::string& label);

    void attach(ShaderProgram* program);

    void reAttach(int stages);

    void detach(int stages);

   private:
    ShaderProgram*& getRef(GLenum stage);
};
}  // namespace GL
