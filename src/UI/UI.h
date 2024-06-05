#pragma once

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#include <nuklear.h>

#include <map>
#include <span>
#include <string>
#include <vector>

// References:
// https://github.dev/Immediate-Mode-UI/Nuklear/blob/master/demo/glfw_opengl4/nuklear_glfw_gl4.h
// https://immediate-mode-ui.github.io/Nuklear/doc/index.html
// https://github.com/Immediate-Mode-UI/Nuklear/wiki
// https://www.thecodingfox.com/nuklear-usage-guide-lwjgl

#pragma region ForwardDecl
#include "../GL/Declarations.h"
class Input;
namespace ui {
class Renderer;
struct Skin;
}  // namespace ui
#pragma endregion

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

struct FontEntry {
    struct FontSize {
        std::string name = "";
        float size = 0;
    };

    std::string filename = "";
    std::vector<FontSize> sizes = {};
};

class FontAtlas {
   private:
    struct nk_font_atlas baker_ = {};
    std::map<std::string, struct nk_font*> fonts_ = {};
    gl::Texture* texture_ = nullptr;
    std::string defaultFont_ = "";
    std::vector<FontEntry> entries_;
    float generatedScale_ = 0.0;

   public:
    FontAtlas(std::initializer_list<FontEntry> entries, std::string default_font);

    ~FontAtlas();

    void generate();

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

    Renderer* renderer_ = nullptr;
    FontAtlas* fontAtlas_ = nullptr;
    Skin* skin_ = nullptr;
    bool hidden_ = false;

   public:
    Backend(FontAtlas* font_atlas, Skin* skin, Renderer* renderer);
    ~Backend();

    void update(Input& input);

    nk_context* context() {
        return &context_;
    }

    FontAtlas* fonts() const {
        return fontAtlas_;
    }

    Skin* skin() const {
        return skin_;
    }

    void setViewport(int width, int height);

    void render();

    void setHidden(bool hidden) {
        this->hidden_ = hidden;
    }

    bool hidden() {
        return this->hidden_;
    }
};

}  // namespace ui
