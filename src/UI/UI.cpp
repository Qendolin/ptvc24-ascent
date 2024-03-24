#include "UI.h"

#include <initializer_list>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_IMPLEMENTATION
#define NK_KEYSTATE_BASED_INPUT
#include <nuklear.h>

#include <glm/glm.hpp>

#include "../GL/StateManager.h"
#include "../Loader/Loader.h"
#include "../Utils.h"

namespace NK {

static float dp_to_px = 1.0f;
static float vw_to_px = 1600.0f / 100.0f;
static float vh_to_px = 900.0f / 100.0f;

void set_scale(int width, int height, float dpi_scale) {
    dp_to_px = dpi_scale * width / 1600.0f;
    vw_to_px = width / 100.0f;
    vh_to_px = height / 100.0f;
}

namespace literals {

float operator"" _dp(long double value) {
    return static_cast<float>(value) * dp_to_px;
}

float operator"" _dp(unsigned long long value) {
    return static_cast<float>(value) * dp_to_px;
}

float operator"" _vw(long double value) {
    return static_cast<float>(value) * vw_to_px;
}

float operator"" _vw(unsigned long long value) {
    return static_cast<float>(value) * vw_to_px;
}

float operator"" _vh(long double value) {
    return static_cast<float>(value) * vh_to_px;
}

float operator"" _vh(unsigned long long value) {
    return static_cast<float>(value) * vh_to_px;
}

}  // namespace literals

FontAtlas::FontAtlas(std::initializer_list<FontEntry> entries, std::string default_font)
    : defaultFont_(default_font) {
    nk_font_atlas_init_default(&baker);
    nk_font_atlas_begin(&baker);

    const float default_height = 16.0f;
    struct nk_font_config config = nk_font_config(default_height);

    for (auto &entry : entries) {
        std::vector<uint8_t> data = Loader::binary(entry.filename);
        for (auto &size : entry.sizes) {
            struct nk_font *font = nk_font_atlas_add_from_memory(&baker, data.data(), data.size(), size.size, &config);
            fonts_[size.name] = font;
        }
    }

    int atlas_width = 0, atlas_height = 0;
    const void *atlas_data = nullptr;
    // TODO: figure out the difference between NK_FONT_ATLAS_ALPHA8 and NK_FONT_ATLAS_RGBA32
    atlas_data = nk_font_atlas_bake(&baker, &atlas_width, &atlas_height, NK_FONT_ATLAS_RGBA32);
    texture_ = new GL::Texture(GL_TEXTURE_2D);
    texture_->allocate(1, GL_RGBA8, atlas_width, atlas_height, 1);
    texture_->load(0, atlas_width, atlas_height, 1, GL_RGBA, GL_UNSIGNED_BYTE, atlas_data);

    // cleanup
    nk_font_atlas_end(&baker, nk_handle_id(texture_->id()), nullptr);
    nk_font_atlas_cleanup(&baker);
}

FontAtlas::~FontAtlas() {
    nk_font_atlas_clear(&baker);
    baker = {};

    if (texture_) {
        texture_->destroy();
        texture_ = nullptr;
    }
}

Backend::Backend(FontAtlas *font_atlas, int max_vertices, int max_indices) : fontAtlas_(font_atlas) {
    if (!nk_init_default(&context_, &font_atlas->defaultFont()->handle)) {
        PANIC("Nuklear initialization failed.");
    }
    nk_style_default(&context_);
    nk_buffer_init_default(&commands_);

    shader_ = new GL::ShaderPipeline({
        new GL::ShaderProgram("assets/shaders/nuklear.vert"),
        new GL::ShaderProgram("assets/shaders/nuklear.frag"),
    });

    sampler_ = new GL::Sampler();
    sampler_->wrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0);
    sampler_->filterMode(GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR);

    vao_ = new GL::VertexArray();
    vao_->layout(0, 0, 2, GL_FLOAT, false, offsetof(struct Vertex, position));
    vao_->layout(0, 1, 2, GL_FLOAT, false, offsetof(struct Vertex, uv));
    vao_->layout(0, 2, 4, GL_UNSIGNED_BYTE, true, offsetof(struct Vertex, color));

    GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    vbo_ = new GL::Buffer();
    size_t vbo_size = sizeof(Vertex) * max_vertices;
    vbo_->allocateEmpty(vbo_size, flags);
    vao_->bindBuffer(0, *vbo_, 0, sizeof(Vertex));
    Vertex *mapped_vbo = reinterpret_cast<Vertex *>(glMapNamedBufferRange(vbo_->id(), 0, vbo_size, flags));
    vertices_ = {mapped_vbo, vbo_size};

    ebo_ = new GL::Buffer();
    size_t ebo_size = sizeof(uint16_t) * max_indices;
    ebo_->allocateEmpty(ebo_size, flags);
    vao_->bindElementBuffer(*ebo_);
    uint16_t *mapped_ebo = reinterpret_cast<uint16_t *>(glMapNamedBufferRange(ebo_->id(), 0, ebo_size, flags));
    indices_ = {mapped_ebo, ebo_size};
}

Backend::~Backend() {
    if (shader_) {
        shader_->destroy();
        shader_ = nullptr;
    }
    if (sampler_) {
        sampler_->destroy();
        sampler_ = nullptr;
    }
    if (vao_) {
        vao_->destroy();
        vao_ = nullptr;
    }
    if (vbo_) {
        glUnmapNamedBuffer(vbo_->id());
        vertices_ = {};
        vbo_->destroy();
        vbo_ = nullptr;
    }
    if (ebo_) {
        glUnmapNamedBuffer(ebo_->id());
        indices_ = {};
        ebo_->destroy();
        ebo_ = nullptr;
    }

    nk_buffer_free(&commands_);
    commands_ = {};

    nk_free(&context_);
    context_ = {};
}

void Backend::update(Input *input) {
    nk_input_begin(&context_);

    if (input->isMouseCaptured()) {
        nk_input_end(&context_);
        return;
    }

    nk_input_key(&context_, NK_KEY_DEL, input->isKeyDown(GLFW_KEY_DELETE));
    nk_input_key(&context_, NK_KEY_ENTER, input->isKeyDown(GLFW_KEY_ENTER));
    nk_input_key(&context_, NK_KEY_TAB, input->isKeyDown(GLFW_KEY_TAB));
    nk_input_key(&context_, NK_KEY_BACKSPACE, input->isKeyDown(GLFW_KEY_BACKSPACE));
    nk_input_key(&context_, NK_KEY_UP, input->isKeyDown(GLFW_KEY_UP));
    nk_input_key(&context_, NK_KEY_DOWN, input->isKeyDown(GLFW_KEY_DOWN));
    nk_input_key(&context_, NK_KEY_TEXT_START, input->isKeyDown(GLFW_KEY_HOME));
    nk_input_key(&context_, NK_KEY_TEXT_END, input->isKeyDown(GLFW_KEY_END));
    nk_input_key(&context_, NK_KEY_SCROLL_START, input->isKeyDown(GLFW_KEY_HOME));
    nk_input_key(&context_, NK_KEY_SCROLL_END, input->isKeyDown(GLFW_KEY_END));
    nk_input_key(&context_, NK_KEY_SCROLL_DOWN, input->isKeyDown(GLFW_KEY_PAGE_DOWN));
    nk_input_key(&context_, NK_KEY_SCROLL_UP, input->isKeyDown(GLFW_KEY_PAGE_UP));
    nk_input_key(&context_, NK_KEY_SHIFT, input->isKeyDown(GLFW_KEY_LEFT_SHIFT) || input->isKeyDown(GLFW_KEY_RIGHT_SHIFT));

    if (input->isKeyDown(GLFW_KEY_LEFT_CONTROL) ||
        input->isKeyDown(GLFW_KEY_RIGHT_CONTROL)) {
        nk_input_key(&context_, NK_KEY_COPY, input->isKeyDown("c"));
        nk_input_key(&context_, NK_KEY_PASTE, input->isKeyDown("v"));
        nk_input_key(&context_, NK_KEY_CUT, input->isKeyDown("x"));
        nk_input_key(&context_, NK_KEY_TEXT_UNDO, input->isKeyDown("z"));
        nk_input_key(&context_, NK_KEY_TEXT_REDO, input->isKeyDown("y"));
        nk_input_key(&context_, NK_KEY_TEXT_WORD_LEFT, input->isKeyDown(GLFW_KEY_LEFT));
        nk_input_key(&context_, NK_KEY_TEXT_WORD_RIGHT, input->isKeyDown(GLFW_KEY_RIGHT));
        // Not implemented: NK_KEY_TEXT_LINE_START, NK_KEY_TEXT_LINE_END
        nk_input_key(&context_, NK_KEY_TEXT_SELECT_ALL, input->isKeyDown("a"));
    } else {
        nk_input_key(&context_, NK_KEY_LEFT, input->isKeyDown(GLFW_KEY_LEFT));
        nk_input_key(&context_, NK_KEY_RIGHT, input->isKeyDown(GLFW_KEY_RIGHT));
        nk_input_key(&context_, NK_KEY_COPY, 0);
        nk_input_key(&context_, NK_KEY_PASTE, 0);
        nk_input_key(&context_, NK_KEY_CUT, 0);
        nk_input_key(&context_, NK_KEY_SHIFT, 0);
    }

    glm::vec2 mouse_pos = input->mousePos();
    glm::vec2 scroll = input->scrollDelta();

    nk_input_motion(&context_, (int)mouse_pos.x, (int)mouse_pos.y);

    nk_input_button(&context_, NK_BUTTON_LEFT, (int)mouse_pos.x, (int)mouse_pos.y, input->isMouseDown(GLFW_MOUSE_BUTTON_LEFT));
    nk_input_button(&context_, NK_BUTTON_MIDDLE, (int)mouse_pos.x, (int)mouse_pos.y, input->isMouseDown(GLFW_MOUSE_BUTTON_MIDDLE));
    nk_input_button(&context_, NK_BUTTON_RIGHT, (int)mouse_pos.x, (int)mouse_pos.y, input->isMouseDown(GLFW_MOUSE_BUTTON_RIGHT));
    // Not implemented: NK_BUTTON_DOUBLE
    nk_input_scroll(&context_, {scroll.x, scroll.y});
    nk_input_end(&context_);
}

void Backend::render(glm::mat4 projection_matrix, glm::ivec2 viewport) {
    GL::pushDebugGroup("Nuklear::Draw");

    struct nk_buffer vertex_buffer = {}, element_buffer = {};
    nk_buffer_init_fixed(&vertex_buffer, vertices_.data(), vertices_.size_bytes());
    nk_buffer_init_fixed(&element_buffer, indices_.data(), indices_.size_bytes());
    nk_convert(&context_, &commands_, &vertex_buffer, &element_buffer, &convertConfig_);

    GL::manager->setEnabled({GL::Capability::Blend, GL::Capability::ScissorTest});
    GL::manager->blendEquation(GL::BlendEquation::FuncAdd);
    GL::manager->blendFunc(GL::BlendFactor::SrcAlpha, GL::BlendFactor::OneMinusSrcAlpha);

    vao_->bind();
    shader_->bind();
    shader_->vertexStage()->setUniform("u_projection_mat", projection_matrix);
    sampler_->bind(0);

    const struct nk_draw_command *cmd;
    const nk_draw_index *offset = NULL;

    nk_draw_foreach(cmd, &context_, &commands_) {
        if (!cmd->elem_count) continue;

        GL::manager->bindTextureUnit(0, cmd->texture.id);
        shader_->fragmentStage()->setUniform("u_use_texture", static_cast<int>(cmd->texture.id != 0));
        GL::manager->setScissor(
            cmd->clip_rect.x,
            viewport.y - (cmd->clip_rect.y + cmd->clip_rect.h),
            cmd->clip_rect.w,
            cmd->clip_rect.h);
        glDrawElements(GL_TRIANGLES, cmd->elem_count, GL_UNSIGNED_SHORT, offset);
        offset += cmd->elem_count;
    }

    GL::manager->bindSampler(0, 0);

    nk_clear(&context_);
    nk_buffer_clear(&commands_);
    GL::popDebugGroup();
}

}  // namespace NK
