
#include "DebugMenu.h"

#include "../../Game.h"

void DebugMenuScreen::draw() {
    using namespace ui::literals;

    Game& game = *Game::instance;
    nk_context* nk = game.ui->context();

    // dark background
    nk->style.window.background = nk_rgba(1, 10, 26, 200);
    nk->style.window.fixed_background = nk_style_item_color(nk_rgba(1, 10, 26, 200));

    nk_style_set_font(nk, &game.ui->fonts()->get("menu_sm")->handle);
    if (nk_begin(nk, "debug_menu", {10_vw, 10_vh, 80_vw, 80_vh}, NK_WINDOW_TITLE | NK_WINDOW_BORDER | NK_WINDOW_SCALABLE | NK_WINDOW_MOVABLE | NK_WINDOW_MINIMIZABLE)) {
        nk_layout_row_dynamic(nk, 30_dp, 1);
        nk_label(nk, "Physics", NK_TEXT_LEFT);
        nk_layout_row_dynamic(nk, 30_dp, 1);

        if (nk_button_label(nk, "Debug Draw")) {
            game.physics->setDebugDrawEnabled(!game.physics->debugDrawEnabled());
        }
    } else {
        close();
    }

    nk_end(nk);
}