#include "Shader.h"

#include "../Utils.h"
#include "StateManager.h"

namespace GL {

std::string readProgramInfoLog(GLuint id) {
    GLint logLength;
    glGetProgramiv(id, GL_INFO_LOG_LENGTH, &logLength);

    std::string log(logLength + 1, '\0');
    glGetProgramInfoLog(id, logLength, nullptr, &log[0]);
    return log;
}

ShaderProgram::ShaderProgram(std::string source, GLenum stage, std::map<std::string, std::string> substitutions)
    : sourceOriginal_(source),
      sourceModified_(source),
      uniformLocations_(),
      stage_(stage) {
    compile(substitutions);
}

ShaderProgram::ShaderProgram(std::string filename, std::map<std::string, std::string> substitutions)
    : uniformLocations_() {
    std::string ext = filename.substr(filename.find_last_of(".") + 1);
    if (ext == "vert") {
        stage_ = GL_VERTEX_SHADER;
    } else if (ext == "frag") {
        stage_ = GL_FRAGMENT_SHADER;
    } else {
        PANIC("Invalid file extension ." + ext);
    }

    sourceOriginal_ = loadFile(filename);
    sourceModified_ = sourceOriginal_;
    compile(substitutions);
}

void ShaderProgram::destroy() {
    if (id_ != 0) {
        glDeleteProgram(id_);
        id_ = 0;
    }
    delete this;
}

std::string ShaderProgram::source() const {
    return sourceModified_;
}

GLuint ShaderProgram::id() const {
    return id_;
}

void ShaderProgram::setDebugLabel(const std::string& label) {
    glObjectLabel(GL_PROGRAM, id_, -1, label.c_str());
}

GLenum ShaderProgram::stage() const {
    return stage_;
}

void ShaderProgram::compile(std::map<std::string, std::string> substitutions) {
    std::string source = sourceOriginal_;
    for (const auto& sub : substitutions) {
        std::string key = sub.first;
        std::string value = sub.second;

        size_t start_pos = 0;
        while ((start_pos = source.find(key, start_pos)) != std::string::npos) {
            source.replace(start_pos, key.length(), value);
            start_pos += value.length();
        }
    }

    const char* cSource = source.c_str();
    GLuint id = glCreateShaderProgramv(stage_, 1, &cSource);
    GLint ok;
    glGetProgramiv(id, GL_LINK_STATUS, &ok);
    if (ok == GL_FALSE) {
        std::string log = readProgramInfoLog(id);
        PANIC("Failed to link shader, log: " + log);
    }
    glValidateProgram(id);
    glGetProgramiv(id, GL_VALIDATE_STATUS, &ok);
    if (ok == GL_FALSE) {
        std::string log = readProgramInfoLog(id);
        PANIC("Failed to validate shader, log: " + log);
    }

    id_ = id;
    sourceModified_ = source;
    uniformLocations_.clear();
}

GLint ShaderProgram::getUniformLocation(const std::string& name) {
    if (uniformLocations_.count(name) > 0) {
        return uniformLocations_[name];
    }

    GLint location = glGetUniformLocation(id_, name.c_str());
    uniformLocations_[name] = location;

    if (location == -1) {
        std::cerr << "Could not get location of " << name << std::endl;
    }

    return location;
}

void setProgramUniform(GLuint prog, GLint location, float value) {
    glProgramUniform1f(prog, location, value);
}

void setProgramUniform(GLuint prog, GLint location, int value) {
    glProgramUniform1i(prog, location, value);
}

void setProgramUniform(GLuint prog, GLint location, const glm::vec2 value) {
    glProgramUniform2f(prog, location, value[0], value[1]);
}

void setProgramUniform(GLuint prog, GLint location, const glm::vec3 value) {
    glProgramUniform3f(prog, location, value[0], value[1], value[2]);
}

void setProgramUniform(GLuint prog, GLint location, const glm::vec4 value) {
    glProgramUniform4f(prog, location, value[0], value[1], value[2], value[3]);
}

void setProgramUniform(GLuint prog, GLint location, const glm::mat3 value) {
    glProgramUniformMatrix3fv(prog, location, 1, GL_FALSE, &value[0][0]);
}

void setProgramUniform(GLuint prog, GLint location, const glm::mat4 value) {
    glProgramUniformMatrix4fv(prog, location, 1, GL_FALSE, &value[0][0]);
}

template <typename T>
void ShaderProgram::setUniform(const std::string& name, T value) {
    GLint location = getUniformLocation(name);
    if (location == -1) {
        return;
    }
    setProgramUniform(id_, location, std::forward<T>(value));
}

template void ShaderProgram::setUniform<int>(const std::string& name, int value);
template void ShaderProgram::setUniform<float>(const std::string& name, float value);
template void ShaderProgram::setUniform<glm::vec2>(const std::string& name, glm::vec2 value);
template void ShaderProgram::setUniform<glm::vec3>(const std::string& name, glm::vec3 value);
template void ShaderProgram::setUniform<glm::vec4>(const std::string& name, glm::vec4 value);
template void ShaderProgram::setUniform<glm::mat3>(const std::string& name, glm::mat3 value);
template void ShaderProgram::setUniform<glm::mat4>(const std::string& name, glm::mat4 value);

template <typename T>
void ShaderProgram::setUniformIndexed(const std::string& name, int index, T value) {
    GLint location = getUniformLocation(name);
    if (location == -1) {
        return;
    }
    setProgramUniform(id_, location + index, std::forward<T>(value));
}

template void ShaderProgram::setUniformIndexed<int>(const std::string& name, int index, int value);
template void ShaderProgram::setUniformIndexed<float>(const std::string& name, int index, float value);
template void ShaderProgram::setUniformIndexed<glm::vec2>(const std::string& name, int index, glm::vec2 value);
template void ShaderProgram::setUniformIndexed<glm::vec3>(const std::string& name, int index, glm::vec3 value);
template void ShaderProgram::setUniformIndexed<glm::vec4>(const std::string& name, int index, glm::vec4 value);
template void ShaderProgram::setUniformIndexed<glm::mat3>(const std::string& name, int index, glm::mat3 value);
template void ShaderProgram::setUniformIndexed<glm::mat4>(const std::string& name, int index, glm::mat4 value);

GLenum shaderStageToBit(GLenum stage) {
    switch (stage) {
        case GL_VERTEX_SHADER:
            return GL_VERTEX_SHADER_BIT;
        case GL_TESS_CONTROL_SHADER:
            return GL_TESS_CONTROL_SHADER_BIT;
        case GL_TESS_EVALUATION_SHADER:
            return GL_TESS_EVALUATION_SHADER_BIT;
        case GL_GEOMETRY_SHADER:
            return GL_GEOMETRY_SHADER_BIT;
        case GL_FRAGMENT_SHADER:
            return GL_FRAGMENT_SHADER_BIT;
        case GL_COMPUTE_SHADER:
            return GL_COMPUTE_SHADER_BIT;
        default:
            PANIC("Invalid stage")
    }
}

ShaderPipeline::ShaderPipeline() {
    glCreateProgramPipelines(1, &id_);
    ownedPrograms_ = {};
}

ShaderPipeline::ShaderPipeline(std::initializer_list<ShaderProgram*> owned_programs) {
    glCreateProgramPipelines(1, &id_);
    attach(owned_programs);
    ownedPrograms_ = owned_programs;
}

void ShaderPipeline::destroy() {
    if (id_ != 0) {
        glDeleteProgramPipelines(1, &id_);
        manager->unbindProgramPipeline(id_);
        id_ = 0;
    }

    for (auto&& p : ownedPrograms_) {
        p->destroy();
    }
    ownedPrograms_ = {};

    delete this;
}

void ShaderPipeline::bind() const {
    manager->bindProgramPipeline(id_);
}

GLuint ShaderPipeline::id() const {
    return id_;
}

ShaderProgram* ShaderPipeline::vertexStage() const {
    return vertStage_;
}

ShaderProgram* ShaderPipeline::fragmentStage() const {
    return fragStage_;
}

void ShaderPipeline::setDebugLabel(const std::string& label) {
    glObjectLabel(GL_PROGRAM_PIPELINE, id_, -1, label.c_str());
}

void ShaderPipeline::attach(const std::initializer_list<ShaderProgram*> programs) {
    for (auto& program : programs) {
        attach(program);
    }
}

void ShaderPipeline::attach(ShaderProgram* program) {
    glUseProgramStages(id_, shaderStageToBit(program->stage()), program->id());
    getRef(program->stage()) = program;
}

void ShaderPipeline::reAttach(int stage) {
    ShaderProgram*& program = getRef(stage);
    if (program == nullptr) {
        glUseProgramStages(id_, shaderStageToBit(stage), 0);
    } else {
        glUseProgramStages(id_, shaderStageToBit(stage), program->id());
    }
}

void ShaderPipeline::detach(int stage) {
    glUseProgramStages(id_, shaderStageToBit(stage), 0);
    getRef(stage) = nullptr;
}

ShaderProgram* ShaderPipeline::get(GLenum stage) const {
    ShaderProgram*& program = const_cast<ShaderPipeline*>(this)->getRef(stage);
    return program;
}

ShaderProgram*& ShaderPipeline::getRef(GLenum stage) {
    switch (stage) {
        case GL_VERTEX_SHADER:
            return vertStage_;
        case GL_TESS_CONTROL_SHADER:
            return tessCtrlStage_;
        case GL_TESS_EVALUATION_SHADER:
            return tessEvalStage_;
        case GL_GEOMETRY_SHADER:
            return geomStage_;
        case GL_FRAGMENT_SHADER:
            return fragStage_;
        case GL_COMPUTE_SHADER:
            return compStage_;
        default:
            PANIC(std::to_string(stage) + " is not a valid shader stage");
    }
}

}  // namespace GL
