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

namespace UI {

static const float REFERENCE_WIDTH = 1600.0f;
static const float REFERENCE_HEIGHT = 900.0f;

static float dp_to_px = 1.0f;
static float vw_to_px = REFERENCE_WIDTH / 100.0f;
static float vh_to_px = REFERENCE_HEIGHT / 100.0f;

void set_scale(int width, int height, float dpi_scale) {
    // should dp only depend on the viewport width?
    dp_to_px = dpi_scale * width / REFERENCE_WIDTH;
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
    nk_font_atlas_init_default(&baker_);
    nk_font_atlas_begin(&baker_);

    const float default_height = 16.0f;
    struct nk_font_config config = nk_font_config(default_height);

    for (auto &entry : entries) {
        std::vector<uint8_t> data = Loader::binary(entry.filename);
        for (auto &size : entry.sizes) {
            struct nk_font *font = nk_font_atlas_add_from_memory(&baker_, data.data(), data.size(), size.size, &config);
            fonts_[size.name] = font;
        }
    }

    int atlas_width = 0, atlas_height = 0;
    const void *atlas_data = nullptr;
    // TODO: figure out the difference between NK_FONT_ATLAS_ALPHA8 and NK_FONT_ATLAS_RGBA32
    atlas_data = nk_font_atlas_bake(&baker_, &atlas_width, &atlas_height, NK_FONT_ATLAS_RGBA32);
    texture_ = new GL::Texture(GL_TEXTURE_2D);
    texture_->setDebugLabel("nk/font");
    texture_->allocate(1, GL_RGBA8, atlas_width, atlas_height, 1);
    texture_->load(0, atlas_width, atlas_height, 1, GL_RGBA, GL_UNSIGNED_BYTE, atlas_data);

    // cleanup
    nk_font_atlas_end(&baker_, nk_handle_id(texture_->id()), nullptr);
    nk_font_atlas_cleanup(&baker_);
}

FontAtlas::~FontAtlas() {
    nk_font_atlas_clear(&baker_);
    baker_ = {};

    if (texture_) {
        texture_->destroy();
        texture_ = nullptr;
    }
}

Backend::Backend(FontAtlas *font_atlas, Skin *skin, Renderer *renderer)
    : fontAtlas_(font_atlas), skin_(skin), renderer_(renderer) {
    if (!nk_init_default(&context_, &font_atlas->defaultFont()->handle)) {
        PANIC("Nuklear initialization failed.");
    }
    nk_style_default(&context_);
    nk_buffer_init_default(&commands_);

    skin->apply(&context_);
}

Backend::~Backend() {
    if (fontAtlas_ != nullptr) {
        delete fontAtlas_;
        fontAtlas_ = nullptr;
    }

    if (renderer_ != nullptr) {
        delete renderer_;
        renderer_ = nullptr;
    }

    if (skin_ != nullptr) {
        delete skin_;
        skin_ = nullptr;
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

}  // namespace UI
