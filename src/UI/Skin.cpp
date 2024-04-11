#include "Skin.h"

#include "../Loader/Loader.h"
#include "../Utils.h"

struct rect {
    int x, y, w, h;
};

struct inset {
    int l, t, r, b;
};

/**
 * [Read more](https://en.wikipedia.org/wiki/9-slice_scaling)
 * @param texture
 * @param tile_size specifies a multipler for the next two parameters
 * @param region specifies a subregion within the texture `(x, y, width, height)`
 * @param inset specifies the insets for the 9-slicing `(left, top, right, bottom)`
 */
struct nk_nine_slice nine_slice(gl::Texture* texture, int tile_size, rect region, inset inset) {
    return nk_sub9slice_id(
        texture->id(),
        static_cast<uint16_t>(texture->width()),
        static_cast<uint16_t>(texture->height()),
        nk_recti(
            region.x * tile_size,
            region.y * tile_size,
            region.w * tile_size,
            region.h * tile_size),
        static_cast<uint16_t>(inset.l * tile_size),
        static_cast<uint16_t>(inset.t * tile_size),
        static_cast<uint16_t>(inset.r * tile_size),
        static_cast<uint16_t>(inset.b * tile_size));
}

struct nk_color rgb(int r, int g, int b) {
    return nk_rgb(r, g, b);
}

namespace ui {

Skin::~Skin() {
    for (auto&& resource : resources) {
        resource->destroy();
    }
}

void Skin::apply(nk_context* nk) {
    nk->style.button.normal = nk_style_item_nine_slice(buttonNormalBackground);
    nk->style.button.hover = nk_style_item_nine_slice(buttonHoverBackground);
    nk->style.button.active = nk_style_item_nine_slice(buttonActiveBackground);
    nk->style.button.text_normal = buttonNormalText;
    nk->style.button.text_hover = buttonHoverText;
    nk->style.button.text_active = buttonActiveText;
}

Skin* loadSkin() {
    Skin* skin = new Skin();
    auto widgets = loader::texture("assets/textures/ui/widgets.png", {.srgb = true});
    skin->resources.push_back(widgets);

    // clang-format off
    skin->buttonNormalBackground = nine_slice(widgets, 16, {0, 4 * 3, 3, 3}, {1, 1, 1, 1});
    skin->buttonHoverBackground  = nine_slice(widgets, 16, {0, 0 * 3, 3, 3}, {1, 1, 1, 1});
    skin->buttonActiveBackground = nine_slice(widgets, 16, {0, 3 * 3, 3, 3}, {1, 1, 1, 1});
	skin->buttonNormalText = rgb(255, 255, 255);
    skin->buttonHoverText  = rgb(255, 255, 255);
    skin->buttonActiveText = rgb(100, 100, 100);
    // clang-format on

    return skin;
}

}  // namespace ui
