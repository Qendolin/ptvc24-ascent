#include "Score.h"

#include "../../Controller/MainMenuController.h"
#include "../../Game.h"
#include "../../Input.h"
#include "../../Util/Format.h"
#include "../../Util/Log.h"
#include "../../Window.h"
#include "../UI.h"

ScoreScreen::ScoreScreen(ScoreEntry score) : score(score) {
    Game::get().input->setMouseMode(Input::MouseMode::Release);
}

ScoreScreen::~ScoreScreen() {
}

void ScoreScreen::draw() {
    using namespace ui::literals;

    Game& game = Game::get();
    nk_context* nk = game.ui->context();

    // transparent background
    nk->style.window.background = nk_rgba(0, 0, 0, 200);
    nk->style.window.fixed_background = nk_style_item_color(nk_rgba(0, 0, 0, 200));
    nk->style.text.color = nk_rgba_f(1, 1, 1, 1);

    if (nk_begin(nk, "score_screen", {0_vw, 0_vh, 100_vw, 100_vh}, 0)) {
        game.tween->step(appearOpacity);

        nk_layout_row_dynamic(nk, 30_dp, 1);
        nk_layout_row_dynamic(nk, 80_dp, 1);
        nk_style_set_font(nk, &game.ui->fonts()->get("menu_lg")->handle);
        nk_label(nk, "Results", NK_TEXT_ALIGN_CENTERED);

        // three column, left and right are empty, middle contains score
        nk_layout_row_dynamic(nk, 100_vh - 300_dp, 3);
        nk_spacer(nk);
        nk->style.window.background = nk_rgba(0, 0, 0, 0);
        nk->style.window.fixed_background = nk_style_item_color(nk_rgba(0, 0, 0, 0));
        if (nk_group_begin(nk, "scores", NK_WINDOW_NO_SCROLLBAR)) {
            nk_style_set_font(nk, &game.ui->fonts()->get("menu_md")->handle);

            // two column layout
            nk_layout_row_dynamic(nk, 30_dp, 2);

            nk_style_push_float(nk, &nk->style.text.padding.x, 5_dp);

            nk_style_push_color(nk, &nk->style.text.color, nk_rgba_f(1, 1, 1, appearOpacity.peek()));
            nk_label(nk, "Time", NK_TEXT_ALIGN_RIGHT);
            std::string time_str = formatTimeRaceClock(score.flight);
            nk_label(nk, time_str.c_str(), NK_TEXT_ALIGN_LEFT);
            nk_style_pop_color(nk);

            nk_style_push_color(nk, &nk->style.text.color, nk_rgba_f(1, 1, 1, appearOpacity.peek() - 1));
            nk_label(nk, "Penalty", NK_TEXT_ALIGN_RIGHT);
            std::string penalty_str = formatTimeRaceClock(score.penalty);
            nk_label(nk, penalty_str.c_str(), NK_TEXT_ALIGN_LEFT);
            nk_style_pop_color(nk);

            nk_spacer(nk);
            nk_spacer(nk);

            nk_style_push_color(nk, &nk->style.text.color, nk_rgba_f(1, 1, 1, appearOpacity.peek() - 3));
            nk_label(nk, "Total", NK_TEXT_ALIGN_RIGHT);
            std::string total_str = formatTimeRaceClock(score.total);
            nk_label(nk, total_str.c_str(), NK_TEXT_ALIGN_LEFT);
            nk_style_pop_color(nk);

            nk_style_pop_float(nk);

            nk_group_end(nk);
        }
        nk_spacer(nk);

        // center button
        nk_layout_row_begin(nk, NK_STATIC, 60_dp, 2);
        nk_layout_row_push(nk, (100_vw - 240_dp) / 2.0f);
        nk_spacer(nk);
        nk_layout_row_push(nk, 240_dp);

        if (nk_button_label(nk, "Exit")) {
            close();
        }
    }

    nk_end(nk);
}