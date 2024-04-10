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

#include "../GL/Texture.h"
#include "../Input.h"
#include "Renderer.h"
#include "Skin.h"

// References:
// https://github.dev/Immediate-Mode-UI/Nuklear/blob/master/demo/glfw_opengl4/nuklear_glfw_gl4.h
// https://immediate-mode-ui.github.io/Nuklear/doc/index.html
// https://github.com/Immediate-Mode-UI/Nuklear/wiki
// https://www.thecodingfox.com/nuklear-usage-guide-lwjgl

namespace ui {

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
    struct nk_font_atlas baker_ = {};
    std::map<std::string, struct nk_font*> fonts_ = {};
    gl::Texture* texture_ = nullptr;
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
    struct nk_context context_ = {};
    struct nk_buffer commands_ = {};
    // used for drawing shapes.
    // TODO: implement the null texture. Or not and remove it.
    struct nk_draw_null_texture nullTexture_ = {};

    Renderer* renderer_ = nullptr;
    FontAtlas* fontAtlas_ = nullptr;
    Skin* skin_ = nullptr;

   public:
    Backend(FontAtlas* font_atlas, Skin* skin, Renderer* renderer);
    ~Backend();

    void update(Input* input);

    nk_context* context() {
        return &context_;
    }

    const FontAtlas* fonts() const {
        return fontAtlas_;
    }

    void setViewport(glm::ivec2 viewport) {
        renderer_->setViewport(viewport);
    }

    void render() {
        renderer_->render(&context_, &commands_);
    }
};

}  // namespace ui
