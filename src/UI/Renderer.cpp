#include "Renderer.h"

#include "../GL/Geometry.h"
#include "../GL/Shader.h"
#include "../GL/StateManager.h"
#include "../GL/Sync.h"
#include "../GL/Texture.h"
#include "../Util/Log.h"

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
    size_t vbo_size = sizeof(Vertex) * max_vertices;
    size_t ebo_size = sizeof(uint16_t) * max_indices;

    vboActive_ = new gl::Buffer();
    vboActive_->setDebugLabel("nk/vbo/a");
    vboActive_->allocateEmpty(vbo_size, flags);
    Vertex *mapped_vbo_write = vboActive_->mapRange<Vertex>(flags);
    verticesActive_ = {mapped_vbo_write, vbo_size};

    vboPassive_ = new gl::Buffer();
    vboPassive_->setDebugLabel("nk/vbo/b");
    vboPassive_->allocateEmpty(vbo_size, flags);
    Vertex *mapped_vbo_read = vboPassive_->mapRange<Vertex>(flags);
    verticesPassive_ = {mapped_vbo_read, vbo_size};

    eboActive_ = new gl::Buffer();
    eboActive_->setDebugLabel("nk/ebo/a");
    eboActive_->allocateEmpty(ebo_size, flags);
    uint16_t *mapped_ebo_write = eboActive_->mapRange<uint16_t>(flags);
    indicesActive_ = {mapped_ebo_write, ebo_size};

    eboPassive_ = new gl::Buffer();
    eboPassive_->setDebugLabel("nk/ebo/b");
    eboPassive_->allocateEmpty(ebo_size, flags);
    uint16_t *mapped_ebo_read = eboPassive_->mapRange<uint16_t>(flags);
    indicesPassive_ = {mapped_ebo_read, ebo_size};

    vao_->bindBuffer(0, *vboActive_, 0, sizeof(Vertex));
    vao_->bindElementBuffer(*eboActive_);

    syncActive_ = new gl::Sync();
    syncPassive_ = new gl::Sync();
}

Renderer::~Renderer() {
    delete shader_;
    delete sampler_;
    delete vao_;
    delete vboActive_;
    delete vboPassive_;
    delete eboActive_;
    delete eboPassive_;
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

    bool can_update = true;
    // Check to see which buffer can be written to. When the `clientWait` call returns `true`
    // we know that the buffer is not currently in use. If it returns `false` we have to try with the other buffer.
    // This ensures that a buffer is not being written while it's being read on the gpu.
    if (syncActive_->clientWait(0)) {
        // the draw command has finished, we can just write to "active" again
    } else {
        if (syncPassive_->clientWait(0)) {
            // the draw command hasn't finished, we cannnot write to current "active" ("active" is still in use)
            // Swap "passive" and "active", the passive buffer will become active and can be written to (it is not in use)
            std::swap(verticesActive_, verticesPassive_);
            std::swap(indicesActive_, indicesPassive_);
            std::swap(vboActive_, vboPassive_);
            std::swap(eboActive_, eboPassive_);
            std::swap(syncActive_, syncPassive_);
            vao_->bindBuffer(0, *vboActive_, 0, sizeof(Vertex));
            vao_->bindElementBuffer(*eboActive_);
        } else {
            // "passive" is also still in use, the pipeline is stalled, can't update either of them now
            // But "passive" is sure to become signaled at some point, so it's not a problem.
            // On the other hand "active" will get it's fence renewd every frame, so it might never
            // be available. For this reason double buffering is used.
            can_update = false;
        }
    }

    if (can_update) {
        struct nk_buffer vertex_buffer = {}, element_buffer = {};
        nk_buffer_init_fixed(&vertex_buffer, verticesActive_.data(), verticesActive_.size_bytes());
        nk_buffer_init_fixed(&element_buffer, indicesActive_.data(), indicesActive_.size_bytes());
        nk_convert(context, commands, &vertex_buffer, &element_buffer, &CONVERT_CONFIG);
    }

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

    syncActive_->fence();

    gl::manager->bindSampler(0, 0);

    nk_clear(context);
    nk_buffer_clear(commands);
    gl::popDebugGroup();
}

}  // namespace ui
