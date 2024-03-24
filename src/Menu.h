#pragma once

#include <tweeny/tweeny.h>

#include "GL/Texture.h"
#include "Loader/Loader.h"
#include "UI/UI.h"
#include "Utils.h"

inline static GL::Texture* image = nullptr;
inline static GL::Texture* widgets = nullptr;
inline static auto title_opacity = tweeny::from(0.0).to(1.0).during(2000);

struct skin {
    struct nk_nine_slice button_normal;
    struct nk_nine_slice button_hover;
    struct nk_nine_slice button_active;
} skin;

void drawMenu(Game* game) {
    using namespace NK::literals;

    nk_context* nk = game->ui->context();

    // window_style_transparent(nk);
    nk->style.button.normal = nk_style_item_nine_slice(skin.button_normal);
    nk->style.button.hover = nk_style_item_nine_slice(skin.button_hover);
    nk->style.button.active = nk_style_item_nine_slice(skin.button_active);
    nk->style.button.text_normal = nk_rgb(255, 255, 255);
    nk->style.button.text_hover = nk_rgb(255, 255, 255);
    nk->style.button.text_active = nk_rgb(100, 100, 100);

    if (nk_begin(nk, "main_menu", {30_vw, 10_vh, 40_vw, 80_vh}, 0)) {
        float img_height = 40_vw * image->height() / image->width();
        nk_layout_row_dynamic(nk, img_height, 1);
        nk_image_color(nk, nk_image_id(image->id()), nk_rgba_f(1.0, 1.0, 1.0, title_opacity.peek()));
        Game::instance->tween(title_opacity);

        // spacing
        nk_layout_row_dynamic(nk, img_height / 2, 1);

        auto used = nk_widget_position(nk);
        auto avail = nk_window_get_content_region_size(nk);
        nk_layout_row_begin(nk, NK_STATIC, avail.y - used.y, 2);
        nk_layout_row_push(nk, (40_vw - 240_dp) / 2.0);
        nk_spacer(nk);
        nk_layout_row_push(nk, 240_dp);

        if (nk_group_begin(nk, "menu_buttons", 0)) {
            nk_style_push_float(nk, &nk->style.window.spacing.y, 50_dp);

            nk_layout_row_dynamic(nk, 60_dp, 1);
            nk_style_set_font(nk, &game->ui->fonts()->get("menu_md")->handle);
            if (nk_button_label(nk, "Play")) {
                LOG("Play pressed");
            }
            if (nk_button_label(nk, "Settings")) {
                LOG("Settings pressed");
            }
            if (nk_button_label(nk, "Quit")) {
                glfwSetWindowShouldClose(game->window, true);
            }

            nk_style_pop_float(nk);  // row sapcing
        }
    }

    nk_end(nk);
}
