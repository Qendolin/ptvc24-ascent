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

    if (nk_begin(nk, "score_screen", {0_vw, 0_vh, 100_vw, 100_vh}, NK_WINDOW_NO_SCROLLBAR)) {
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

            nk_style_pop_float(nk);  // text padding

            // performance graph
            nk_layout_row_dynamic(nk, 60_dp, 1);
            nk_spacer(nk);  // spacing
            if (appearOpacity.progress() == 1.0) {
                drawPerformance_();
            }

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

void ScoreScreen::drawPerformance_() {
    using namespace ui::literals;

    Game& game = Game::get();
    nk_context* nk = game.ui->context();

    auto& scores = *Game::get().scores;
    auto& recent = Game::get().scores->recentScores();

    const ScoreEntry& hi_score = scores.lastHighScore();

    nk_layout_row_dynamic(nk, 40_dp, 1);
    nk_label(nk, "Performance", NK_TEXT_ALIGN_LEFT);
    nk_layout_row_dynamic(nk, 180_dp, 1);
    auto graph_space = nk_window_get_content_region(nk);
    nk_widget(&graph_space, nk);
    auto graph_inner = nk_rect(graph_space.x + 15_dp, graph_space.y + 15_dp, graph_space.w - 60_dp, graph_space.h - 30_dp);
    auto canvas = nk_window_get_canvas(nk);

    game.tween->step(graphProgress);
    // background
    nk_fill_rect(canvas, graph_space, 5_dp, nk_rgba_f(1, 1, 1, 0.2f * graphProgress.peek()));

    auto scissor_rect = graph_space;
    scissor_rect.w *= graphProgress.peek();
    auto prev_scissor = canvas->clip;
    nk_push_scissor(canvas, scissor_rect);

    float graph_max = std::max_element(recent.begin(), recent.end(), [](ScoreEntry const& a, ScoreEntry const& b) {
                          return a.total < b.total;
                      })->total;
    float graph_min = scores.highScore().total;  // use current hi score
    float graph_range = graph_max - graph_min;
    if (graph_range == 0.0) graph_range = 1.0;

    // hi score
    if (graphProgress.progress() == 1.0 && hi_score.valid) {
        float hi_y = (1.0f - (hi_score.total - graph_min) / graph_range) * graph_inner.h;
        nk_stroke_line(canvas, graph_inner.x, graph_inner.y + hi_y, graph_inner.x + graph_inner.w + 10_dp, graph_inner.y + hi_y, 1.0, nk_rgba_f(1.0, 1.0, 1.0, 0.5));
        nk_draw_text(canvas, nk_rect(graph_inner.x + graph_inner.w + 15_dp, graph_inner.y + hi_y - 12_dp, 30_dp, 20_dp), "Hi", 2, &game.ui->fonts()->get("menu_ty")->handle, nk_rgba(0, 0, 0, 0), nk_rgb_f(1, 1, 1));
    }

    int limited_count = static_cast<int>(std::max(std::min(recent.size() - 1, 25ULL), 1ULL));
    float x_scale = 1.0f / static_cast<float>(limited_count);
    for (int i = 0; i < limited_count; i++) {
        const ScoreEntry& curr = recent[i];
        const ScoreEntry& prev = i + 1 == recent.size() ? curr : recent[i + 1];
        float y0 = (1.0f - (prev.total - graph_min) / graph_range) * graph_inner.h;
        float y1 = (1.0f - (curr.total - graph_min) / graph_range) * graph_inner.h;
        float x0 = (1.0f - (i + 1) * x_scale) * graph_inner.w;
        float x1 = (1.0f - i * x_scale) * graph_inner.w;
        nk_stroke_line(canvas, graph_inner.x + x0, graph_inner.y + y0, graph_inner.x + x1, graph_inner.y + y1, 2.0f, nk_rgba_f(1, 1, 1, 1));
        if (i == 0 && graphProgress.progress() == 1.0) {
            // encircle current score
            nk_stroke_circle(canvas, nk_recta(nk_vec2(graph_inner.x + x1 - 7, graph_inner.y + y1 - 7), nk_vec2(14, 14)), 2.0, nk_rgba_f(1, 1, 1, 1));
        }
    }
    nk_push_scissor(canvas, prev_scissor);

    nk_layout_row_dynamic(nk, 10_dp, 1);
    nk_spacer(nk);

    nk_layout_row_dynamic(nk, 30_dp, 2);
    nk_label(nk, "High Score", NK_TEXT_ALIGN_LEFT);
    std::string total_str = formatTimeRaceClock(hi_score.total);
    nk_label(nk, total_str.c_str(), NK_TEXT_ALIGN_RIGHT);
}