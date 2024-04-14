#include "Renderer.h"

#include "../GL/Geometry.h"
#include "../GL/Shader.h"
#include "../GL/StateManager.h"
#include "../GL/Texture.h"
#include "../Utils.h"

namespace ui {

static const struct nk_draw_vertex_layout_element VERTEX_LAYOUT[] = {
    {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(Renderer::Vertex, position)},
    {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(Renderer::Vertex, uv)},
    {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(Renderer::Vertex, color)},
    {NK_VERTEX_LAYOUT_END}};

static const struct nk_convert_config CONVERT_CONFIG = {
    .global_alpha = 1,
    .line_AA = NK_ANTI_ALIASING_ON,
    .shape_AA = NK_ANTI_ALIASING_ON,
    .circle_segment_count = 22,
    .arc_segment_count = 22,
    .curve_segment_count = 22,
    .tex_null = {.texture = 0, .uv = {0, 0}},
    .vertex_layout = VERTEX_LAYOUT,
    .vertex_size = sizeof(Renderer::Vertex),
    .vertex_alignment = NK_ALIGNOF(Renderer::Vertex),
};

Renderer::Renderer(int max_vertices, int max_indices) {
    shader_ = new gl::ShaderPipeline({
        new gl::ShaderProgram("assets/shaders/nuklear.vert"),
        new gl::ShaderProgram("assets/shaders/nuklear.frag"),
    });
    shader_->setDebugLabel("nk/shader");

    sampler_ = new gl::Sampler();
    sampler_->setDebugLabel("nk/sampler");
    sampler_->wrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0);
    sampler_->filterMode(GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR);

    vao_ = new gl::VertexArray();
    vao_->setDebugLabel("nk/vao");
    vao_->layout(0, 0, 2, GL_FLOAT, false, offsetof(struct Vertex, position));
    vao_->layout(0, 1, 2, GL_FLOAT, false, offsetof(struct Vertex, uv));
    vao_->layout(0, 2, 4, GL_UNSIGNED_BYTE, true, offsetof(struct Vertex, color));

    GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    vbo_ = new gl::Buffer();
    vbo_->setDebugLabel("nk/vbo");
    size_t vbo_size = sizeof(Vertex) * max_vertices;
    vbo_->allocateEmpty(vbo_size, flags);
    vao_->bindBuffer(0, *vbo_, 0, sizeof(Vertex));
    Vertex *mapped_vbo = vbo_->mapRange<Vertex>(flags);
    vertices_ = {mapped_vbo, vbo_size};

    ebo_ = new gl::Buffer();
    ebo_->setDebugLabel("nk/ebo");
    size_t ebo_size = sizeof(uint16_t) * max_indices;
    ebo_->allocateEmpty(ebo_size, flags);
    vao_->bindElementBuffer(*ebo_);
    uint16_t *mapped_ebo = ebo_->mapRange<uint16_t>(flags);
    indices_ = {mapped_ebo, ebo_size};
}

Renderer::~Renderer() {
    delete shader_;
    delete sampler_;
    delete vao_;
    delete vbo_;
    delete ebo_;
}

void Renderer::setViewport(int width, int height) {
    this->viewport_ = {width, height};
    float w = static_cast<float>(width);
    float h = static_cast<float>(height);
    // clang-format off
    projectionMatrix_ = glm::mat4(
        2 / w,      0,    0,    0,
            0, -2 / h,    0,    0,
            0,      0,    1,    0,
           -1,      1,    0,    1);
    // clang-format on
}

void Renderer::render(struct nk_context *context, struct nk_buffer *commands) {
    if (viewport_ == glm::ivec2(0, 0)) {
        PANIC("UI renderer viewport not set");
    }

    gl::pushDebugGroup("Nuklear::Draw");

    struct nk_buffer vertex_buffer = {}, element_buffer = {};
    nk_buffer_init_fixed(&vertex_buffer, vertices_.data(), vertices_.size_bytes());
    nk_buffer_init_fixed(&element_buffer, indices_.data(), indices_.size_bytes());
    nk_convert(context, commands, &vertex_buffer, &element_buffer, &CONVERT_CONFIG);

    gl::manager->setEnabled({gl::Capability::Blend, gl::Capability::ScissorTest});
    gl::manager->blendEquation(gl::BlendEquation::FuncAdd);
    gl::manager->blendFunc(gl::BlendFactor::SrcAlpha, gl::BlendFactor::OneMinusSrcAlpha);

    vao_->bind();
    shader_->bind();
    shader_->vertexStage()->setUniform("u_projection_mat", projectionMatrix_);
    sampler_->bind(0);

    const struct nk_draw_command *cmd;
    const nk_draw_index *offset = NULL;

    nk_draw_foreach(cmd, context, commands) {
        if (!cmd->elem_count) continue;

        gl::manager->bindTextureUnit(0, cmd->texture.id);
        shader_->fragmentStage()->setUniform("u_use_texture", static_cast<int>(cmd->texture.id != 0));
        gl::manager->setScissor(
            static_cast<int>(cmd->clip_rect.x),
            // flip y
            viewport_.y - static_cast<int>(cmd->clip_rect.y + cmd->clip_rect.h),
            static_cast<int>(cmd->clip_rect.w),
            static_cast<int>(cmd->clip_rect.h));
        glDrawElements(GL_TRIANGLES, cmd->elem_count, GL_UNSIGNED_SHORT, offset);
        offset += cmd->elem_count;
    }

    gl::manager->bindSampler(0, 0);

    nk_clear(context);
    nk_buffer_clear(commands);
    gl::popDebugGroup();
}

}  // namespace ui
