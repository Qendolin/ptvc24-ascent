#pragma once

#include <glm/glm.hpp>
#include <span>

#include "../GL/Declarations.h"
#include "UI.h"

class Input;

namespace ui {

// Executes the nuklear draw commands
// References:
//   https://immediate-mode-ui.github.io/Nuklear/doc/index.html#nuklear/api/drawing
//   https://github.com/Immediate-Mode-UI/Nuklear/blob/055a0aad0ff10921d638a9b93b0fe87f3a86d777/demo/glfw_opengl4/nuklear_glfw_gl4.h#L361
class Renderer {
   public:
    struct Vertex {
        glm::vec2 position;
        glm::vec2 uv;
        // packed rgba color
        uint32_t color;
    };

   private:
    gl::ShaderPipeline* shader_ = nullptr;
    gl::Sampler* sampler_ = nullptr;
    gl::VertexArray* vao_ = nullptr;
    // Use double buffering.
    gl::Buffer* vboActive_ = nullptr;
    gl::Buffer* vboPassive_ = nullptr;
    // points into the vbo using glMapNamedBufferRange
    std::span<Vertex> verticesActive_ = {};
    std::span<Vertex> verticesPassive_ = {};
    gl::Buffer* eboActive_ = nullptr;
    gl::Buffer* eboPassive_ = nullptr;
    // points into the ebo using glMapNamedBufferRange
    std::span<uint16_t> indicesActive_ = {};
    std::span<uint16_t> indicesPassive_ = {};
    gl::Sync* syncActive_ = nullptr;
    gl::Sync* syncPassive_ = nullptr;

    glm::mat4 projectionMatrix_ = glm::mat4(1.0);
    glm::ivec2 viewport_ = glm::ivec2(0, 0);

   public:
    Renderer(int max_vertices = 4 * 1024, int max_indices = 4 * 1024);
    ~Renderer();

    void setViewport(int width, int height);

    void render(struct nk_context* context, struct nk_buffer* commands);
};

}  // namespace ui
