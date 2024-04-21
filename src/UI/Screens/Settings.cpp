#include "Settings.h"

#include "../../Game.h"
#include "../../Input.h"
#include "../../Window.h"
#include "../UI.h"

void SettingsScreen::open(Settings settings) {
    opened_ = true;
    settings_ = settings;
}

void SettingsScreen::draw_() {
    using namespace ui::literals;

    Game& game = Game::get();
    nk_context* nk = game.ui->context();

    // transparent background
    nk->style.window.background = nk_rgba(0, 0, 0, 0);
    nk->style.window.fixed_background = nk_style_item_color(nk_rgba(0, 0, 0, 0));
    nk->style.text.color = nk_rgba_f(1, 1, 1, 1);
    nk_style_set_font(nk, &game.ui->fonts()->get("menu_lg")->handle);

    if (nk_begin(nk, "settings_menu", {30_vw, 10_vh, 40_vw, 80_vh}, NK_WINDOW_NO_SCROLLBAR)) {
        nk_style_set_font(nk, &game.ui->fonts()->get("menu_lg")->handle);

        nk_layout_row_dynamic(nk, 80_dp, 1);
        nk_label(nk, "Settings", NK_TEXT_ALIGN_CENTERED);

        // spacing
        nk_layout_row_dynamic(nk, 80_dp, 1);
        nk_spacer(nk);

        auto font_md = &game.ui->fonts()->get("menu_md")->handle;
        auto font_sm = &game.ui->fonts()->get("menu_sm")->handle;
        nk_style_set_font(nk, font_md);

        // prepare sublayout for settings
        auto used = nk_widget_position(nk);
        auto avail = nk_window_get_content_region_size(nk);
        nk_layout_row_dynamic(nk, avail.y - used.y - 60_dp, 1);

        if (nk_group_begin(nk, "settings", 0)) {
            nk_layout_row_template_begin(nk, 40_dp);
            nk_layout_row_template_push_static(nk, 240_dp);
            nk_layout_row_template_push_dynamic(nk);
            nk_layout_row_template_push_static(nk, 60_dp);
            nk_layout_row_template_end(nk);

            nk_style_set_font(nk, font_md);
            nk_label(nk, "FOV", NK_TEXT_ALIGN_LEFT);
            nk_slider_float(nk, 70, &settings_.fov, 130, 5);
            nk_style_set_font(nk, font_sm);
            nk_labelf(nk, NK_TEXT_ALIGN_RIGHT, "%.0f", settings_.fov);

            nk_style_set_font(nk, font_md);
            nk_label(nk, "Sensitivity", NK_TEXT_ALIGN_LEFT);
            nk_slider_float(nk, 0.05f, &settings_.lookSensitivity, 0.666f, 0.001f);
            nk_style_set_font(nk, font_sm);
            nk_labelf(nk, NK_TEXT_ALIGN_RIGHT, "%.3f", settings_.lookSensitivity);

            nk_group_end(nk);
        }

        nk_layout_row_template_begin(nk, 60_dp);
        nk_layout_row_template_push_static(nk, 240_dp);
        nk_layout_row_template_push_dynamic(nk);
        nk_layout_row_template_push_static(nk, 240_dp);
        nk_layout_row_template_end(nk);

        if (nk_button_label(nk, "Discard")) {
            close();
        }
        nk_spacer(nk);
        if (nk_button_label(nk, "Apply")) {
            game.settings.set(settings_);
            game.settings.save();
            close();
        }
    }

    nk_end(nk);
}