#pragma once
#include <glm/glm.hpp>
#include <map>
#include <string>
#include <vector>

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
    ShaderProgram(std::string source, GLenum stage, std::map<std::string, std::string> substitutions = {});
    ShaderProgram(std::string filename, std::map<std::string, std::string> substitutions = {});

    ~ShaderProgram() {
        checkDestroyed(GL_PROGRAM);
    }

    void destroy();

    std::string source() const;

    GLuint id() const;

    GLenum stage() const;

    void compile(std::map<std::string, std::string> defs = {});

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
    std::initializer_list<ShaderProgram*> owned_programs_;

   public:
    ShaderPipeline();
    ShaderPipeline(std::initializer_list<ShaderProgram*> owned_programs);

    ~ShaderPipeline() {
        checkDestroyed(GL_PROGRAM_PIPELINE);
        for (auto&& p : owned_programs_) {
            delete p;
        }
        owned_programs_ = {};
    }

    void destroy();

    void bind() const;

    GLuint id() const;

    ShaderProgram* vertexStage() const;

    ShaderProgram* fragmentStage() const;

    ShaderProgram* get(GLenum stage) const;

    void setDebugLabel(const std::string& label);

    void attach(const std::initializer_list<ShaderProgram*> programs);

    void attach(ShaderProgram* program);

    void reAttach(int stages);

    void detach(int stages);

   private:
    ShaderProgram*& getRef(GLenum stage);
};
}  // namespace GL
