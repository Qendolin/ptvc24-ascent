#pragma once

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#include <nuklear.h>

#include <span>

#include "../GL/Geometry.h"
#include "../GL/Shader.h"
#include "../GL/Texture.h"
#include "../Input.h"

namespace ui {

// Executes the nuklear draw commands
// References:
//   https://immediate-mode-ui.github.io/Nuklear/doc/index.html#nuklear/api/drawing
//   https://github.com/Immediate-Mode-UI/Nuklear/blob/055a0aad0ff10921d638a9b93b0fe87f3a86d777/demo/glfw_opengl4/nuklear_glfw_gl4.h#L361
class Renderer {
   public:
    typedef struct Vertex {
        glm::vec2 position;
        glm::vec2 uv;
        // packed rgba color
        uint32_t color;
    } Vertex;

   private:
    gl::ShaderPipeline* shader_ = nullptr;
    gl::Sampler* sampler_ = nullptr;
    gl::VertexArray* vao_ = nullptr;
    gl::Buffer* vbo_ = nullptr;
    // points into the vbo using glMapNamedBufferRange
    std::span<Vertex> vertices_ = {};
    gl::Buffer* ebo_ = nullptr;
    // points into the ebo using glMapNamedBufferRange
    std::span<uint16_t> indices_ = {};

    glm::mat4 projectionMatrix_ = glm::mat4(1.0);
    glm::ivec2 viewport = glm::ivec2(0, 0);

    inline static const struct nk_draw_vertex_layout_element VERTEX_LAYOUT_[] = {
        {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(Vertex, position)},
        {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(Vertex, uv)},
        {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(Vertex, color)},
        {NK_VERTEX_LAYOUT_END}};

    struct nk_convert_config convertConfig_ = {
        .global_alpha = 1.0f,
        .line_AA = NK_ANTI_ALIASING_ON,
        .shape_AA = NK_ANTI_ALIASING_ON,
        .circle_segment_count = 22,
        .arc_segment_count = 22,
        .curve_segment_count = 22,
        .tex_null = {.texture = 0, .uv = {0, 0}},
        .vertex_layout = VERTEX_LAYOUT_,
        .vertex_size = sizeof(Vertex),
        .vertex_alignment = NK_ALIGNOF(Vertex),
    };

   public:
    Renderer(int max_vertices = 4 * 1024, int max_indices = 4 * 1024);
    ~Renderer();

    /**
     * @param viewport the viewport size
     */
    void setViewport(glm::ivec2 viewport);

    void render(struct nk_context* context, struct nk_buffer* commands);
};

}  // namespace ui
