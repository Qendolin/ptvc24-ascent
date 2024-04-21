#include "Start.h"

#include "../../Game.h"
#include "../../Input.h"
#include "../../Window.h"
#include "../UI.h"

void StartScreen::open() {
    opened_ = true;
    Game::get().input->setMouseMode(Input::MouseMode::Capture);
}

void StartScreen::draw_() {
    using namespace ui::literals;

    Game& game = Game::get();
    nk_context* nk = game.ui->context();

    // transparent background
    nk->style.window.background = nk_rgba(0, 0, 0, 120);
    nk->style.window.fixed_background = nk_style_item_color(nk_rgba(0, 0, 0, 120));
    nk->style.text.color = nk_rgba_f(1, 1, 1, 1);

    if (nk_begin(nk, "start_screen", {0_vw, 60_vh, 100_vw, 80_dp}, NK_WINDOW_NO_SCROLLBAR)) {
        nk_style_set_font(nk, &game.ui->fonts()->get("menu_lg")->handle);
        nk_layout_row_dynamic(nk, 80_dp, 1);

        nk_label(nk, "Press Space", NK_TEXT_ALIGN_CENTERED);

        if (game.input->isKeyPress(GLFW_KEY_SPACE)) {
            close();
        }
    }

    nk_end(nk);
}