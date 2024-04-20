#include "Pause.h"

#include "../../Game.h"
#include "../../Input.h"
#include "../../Window.h"
#include "../UI.h"

PauseScreen::PauseScreen() {
    Game::get().input->setMouseMode(Input::MouseMode::Release);
    Game::get().input->centerMouse();
}

PauseScreen::~PauseScreen() {
}

void PauseScreen::draw() {
    using namespace ui::literals;

    Game& game = Game::get();
    nk_context* nk = game.ui->context();

    // transparent background
    nk->style.window.background = nk_rgba(0, 0, 0, 120);
    nk->style.window.fixed_background = nk_style_item_color(nk_rgba(0, 0, 0, 120));
    nk->style.text.color = nk_rgba_f(1, 1, 1, 1);

    if (nk_begin(nk, "pause_screen", {0_vw, 0_vh, 100_vw, 100_vh}, NK_WINDOW_NO_SCROLLBAR)) {
        nk_style_set_font(nk, &game.ui->fonts()->get("menu_lg")->handle);
        nk_layout_row_dynamic(nk, 80_dp, 1);
        nk_spacer(nk);

        nk_layout_row_dynamic(nk, 80_dp, 1);
        nk_label(nk, "Paused", NK_TEXT_ALIGN_CENTERED);

        // spacing
        nk_layout_row_dynamic(nk, 160_dp, 1);
        nk_spacer(nk);

        // prepare sublayout for buttons
        auto used = nk_widget_position(nk);
        auto avail = nk_window_get_content_region_size(nk);
        nk_layout_row_begin(nk, NK_STATIC, avail.y - used.y, 2);
        nk_layout_row_push(nk, (100_vw - 240_dp) / 2.0f);
        nk_spacer(nk);
        nk_layout_row_push(nk, 240_dp);

        nk->style.window.background = nk_rgba(0, 0, 0, 0);
        nk->style.window.fixed_background = nk_style_item_color(nk_rgba(0, 0, 0, 0));
        nk_style_set_font(nk, &game.ui->fonts()->get("menu_md")->handle);

        if (nk_group_begin(nk, "pause_buttons", NK_WINDOW_NO_SCROLLBAR)) {
            // row spacing
            nk_style_push_float(nk, &nk->style.window.spacing.y, 50_dp);

            nk_layout_row_dynamic(nk, 60_dp, 1);
            if (nk_button_label(nk, "Resume")) {
                close();
            }
            if (nk_button_label(nk, "Respawn")) {
                action = Action::Respawn;
                close();
            }
            if (nk_button_label(nk, "Exit")) {
                action = Action::Exit;
                close();
            }

            nk_style_pop_float(nk);  // row sapcing
            nk_group_end(nk);
        }

        if (game.input->isKeyPress(GLFW_KEY_ESCAPE)) {
            close();
        }
    }

    nk_end(nk);
}

void PauseScreen::close() {
    Screen::close();
    Game::get().input->setMouseMode(Input::MouseMode::Capture);
}