#pragma once
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#include <nuklear.h>

#include <map>
#include <span>

#include "../GL/Geometry.h"
#include "../GL/Shader.h"
#include "../GL/Texture.h"
#include "../Input.h"

// References:
// https://github.dev/Immediate-Mode-UI/Nuklear/blob/master/demo/glfw_opengl4/nuklear_glfw_gl4.h
// https://immediate-mode-ui.github.io/Nuklear/doc/index.html
// https://github.com/Immediate-Mode-UI/Nuklear/wiki
// https://www.thecodingfox.com/nuklear-usage-guide-lwjgl

namespace NK {

void set_scale(int width, int height, float dpi_scale);

// These literal operators help with designing gui's that scale across different
// window sizes and monitor resolutions.
namespace literals {

float operator"" _dp(long double value);

// dynamic point
// 1 dp equals 1 pixel for a 1600x900 reference size.
// On high resoltuon displays it will be more than one pixel.
// On high dpi displays it will also be more than 1 pixel.
float operator"" _dp(unsigned long long value);

float operator"" _vw(long double value);

// viewport width
// 100 vw equals the entire width of the viewport
float operator"" _vw(unsigned long long value);

float operator"" _vh(long double value);

// viewport height
// 100 vh equals the entire height of the viewport
float operator"" _vh(unsigned long long value);

}  // namespace literals

typedef struct Vertex {
    glm::vec2 position;
    glm::vec2 uv;
    uint32_t color;
} Vertex;

typedef struct FontEntry {
    struct FontSize {
        std::string name = "";
        float size = 0;
    };

    std::string filename = "";
    std::vector<FontSize> sizes = {};
} FontEntry;

class FontAtlas {
   private:
    struct nk_font_atlas baker = {};
    std::map<std::string, struct nk_font*> fonts_ = {};
    GL::Texture* texture_ = nullptr;
    std::string defaultFont_ = "";

   public:
    FontAtlas(std::initializer_list<FontEntry> entries, std::string default_font);

    ~FontAtlas();

    struct nk_font* defaultFont() const {
        return fonts_.at(defaultFont_);
    }

    struct nk_font* get(std::string name) const {
        return fonts_.at(name);
    }
};

class Backend {
   private:
    inline static const struct nk_draw_vertex_layout_element VERTEX_LAYOUT_[] = {
        {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct Vertex, position)},
        {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct Vertex, uv)},
        {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct Vertex, color)},
        {NK_VERTEX_LAYOUT_END}};

    struct nk_context context_ = {};
    struct nk_buffer commands_ = {};
    // used for drawing shapes.
    // TODO:
    struct nk_draw_null_texture nullTexture_ = {};
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
        .vertex_alignment = NK_ALIGNOF(struct Vertex),
    };

    GL::ShaderPipeline* shader_ = nullptr;
    GL::Sampler* sampler_ = nullptr;
    GL::VertexArray* vao_ = nullptr;
    GL::Buffer* vbo_ = nullptr;
    // points into the vbo using glMapNamedBufferRange
    std::span<Vertex> vertices_ = {};
    GL::Buffer* ebo_ = nullptr;
    // points into the ebo using glMapNamedBufferRange
    std::span<uint16_t> indices_ = {};

    FontAtlas* fontAtlas_ = nullptr;

   public:
    Backend(FontAtlas* font_atlas, int max_vertices = 4 * 1024, int max_indices = 4 * 1024);
    ~Backend();

    void update(Input* input);

    /**
     * @param projection_matrix the orthographic projection matrix
     * @param viewport the viewport size
     */
    void render(glm::mat4 projection_matrix, glm::ivec2 viewport);

    nk_context* context() {
        return &context_;
    }

    const FontAtlas* fonts() const {
        return fontAtlas_;
    }
};

}  // namespace NK
