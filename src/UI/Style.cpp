#include "Style.h"

#include "../Loader/Loader.h"

typedef struct rect {
    int x, y, w, h;
} rect;

typedef struct inset {
    int l, t, r, b;
} inset;

/**
 * [Read more](https://en.wikipedia.org/wiki/9-slice_scaling)
 * @param texture
 * @param tile_size specifies a multipler for the next two parameters
 * @param region specifies a subregion within the texture `(x, y, width, height)`
 * @param inset specifies the insets for the 9-slicing `(left, top, right, bottom)`
 */
struct nk_nine_slice nine_slice(GL::Texture* texture, int tile_size, rect region, inset inset) {
    return nk_sub9slice_id(
        texture->id(),
        texture->width(),
        texture->height(),
        nk_rect(
            region.x * tile_size,
            region.y * tile_size,
            region.w * tile_size,
            region.h * tile_size),
        inset.l * tile_size,
        inset.t * tile_size,
        inset.r * tile_size,
        inset.b * tile_size);
}

struct nk_color rgb(int r, int g, int b) {
    return nk_rgb(r, g, b);
}

Skin load_skin() {
    auto widgets = Loader::texture("assets/textures/ui/widgets.png", {.srgb = true});

    Skin skin = {};

    // clang-format off
    skin.button_normal_background = nine_slice(widgets, 16, {0, 4 * 3, 3, 3}, {1, 1, 1, 1});
    skin.button_hover_background  = nine_slice(widgets, 16, {0, 0 * 3, 3, 3}, {1, 1, 1, 1});
    skin.button_active_background = nine_slice(widgets, 16, {0, 3 * 3, 3, 3}, {1, 1, 1, 1});
	skin.button_normal_text = rgb(255, 255, 255);
    skin.button_hover_text  = rgb(255, 255, 255);
    skin.button_active_text = rgb(100, 100, 100);
    // clang-format on

    return skin;
}

void Skin::apply(nk_context* nk) {
    nk->style.button.normal = nk_style_item_nine_slice(button_normal_background);
    nk->style.button.hover = nk_style_item_nine_slice(button_hover_background);
    nk->style.button.active = nk_style_item_nine_slice(button_active_background);
    nk->style.button.text_normal = button_normal_text;
    nk->style.button.text_hover = button_hover_text;
    nk->style.button.text_active = button_active_text;
}

void window_style_transparent(nk_context* nk) {
    nk->style.window.background = nk_rgba(0, 0, 0, 0);
    nk->style.window.fixed_background = nk_style_item_color(nk_rgba(0, 0, 0, 0));
}