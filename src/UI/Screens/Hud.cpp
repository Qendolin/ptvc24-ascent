#include "Hud.h"

#include "../../Controller/MainController.h"
#include "../../Controller/RaceManager.h"
#include "../../GL/Texture.h"
#include "../../Game.h"
#include "../../Input.h"
#include "../../Loader/Loader.h"
#include "../../Scene/Character.h"
#include "../../Util/Format.h"
#include "../Skin.h"
#include "../UI.h"

using namespace ui::literals;

Hud::Hud() {
    crosshairImage_ = loader::texture("assets/textures/ui/crosshair.png", {.srgb = true});
}

Hud::~Hud() {
    delete crosshairImage_;
}

void Hud::drawTimer_(Game &game, struct nk_context *nk, struct nk_rect &bounds) {
    MainController &controller = dynamic_cast<MainController &>(*game.controller);
    RaceManager &race_manager = controller.raceManager;
    const ui::Skin &skin = *game.ui->skin();

    const float timer_height = 90_dp;
    const float timer_width = 260_dp;
    nk_layout_space_push(nk, nk_rect(bounds.w / 2 - bounds.w / 5 - timer_width, bounds.h / 2 - timer_height / 2, timer_width, timer_height));
    nk->style.window.background = nk_rgba_f(0, 0, 0, 0);
    nk->style.window.fixed_background = nk_style_item_color(nk_rgba_f(0, 0, 0, 0));
    nk_style_push_vec2(nk, &nk->style.window.group_padding, nk_vec2(0, 0));
    nk_style_push_vec2(nk, &nk->style.window.spacing, nk_vec2(0, 0));
    if (nk_group_begin(nk, "timer", NK_WINDOW_NO_SCROLLBAR)) {
        auto canvas = nk_window_get_canvas(nk);

        // Timer
        // background
        nk_layout_row_static(nk, 80_dp, static_cast<int>(timer_width), 1);
        struct nk_rect timer_bounds;
        nk_widget(&timer_bounds, nk);
        nk_draw_nine_slice(canvas, timer_bounds, &skin.timerBackground, nk_rgba_f(1, 1, 1, 1));
        nk->active->layout->at_y -= nk->active->layout->row.height;  // roll-back position

        // First row
        nk_layout_row_begin(nk, NK_STATIC, 45_dp, 2);

        // Checkpoint
        nk_style_push_vec2(nk, &nk->style.text.padding, nk_vec2(0_dp, 0_dp));
        nk_layout_row_push(nk, timer_width - 160_dp);
        nk_style_set_font(nk, &game.ui->fonts()->get("menu_sm")->handle);
        // nk_layout_row_static(nk, 45_dp, static_cast<int>(timer_width), 1);
        std::string checkpoint_str = std::format("{}/{}", race_manager.checkpointIndex() + 1, race_manager.checkpointCount());
        nk_label_colored(nk, checkpoint_str.c_str(), NK_TEXT_RIGHT, nk_rgb(28, 28, 28));
        nk_style_pop_vec2(nk);

        // Timer
        nk_style_push_vec2(nk, &nk->style.text.padding, nk_vec2(5_dp, 0_dp));
        nk_layout_row_push(nk, 160_dp);
        nk_style_set_font(nk, &game.ui->fonts()->get("menu_md")->handle);
        // nk_layout_row_static(nk, 45_dp, static_cast<int>(timer_width), 1);
        std::string time_str = formatTimeRaceClock(race_manager.timer());
        nk_label_colored(nk, time_str.c_str(), NK_TEXT_RIGHT, nk_rgb(28, 28, 28));
        nk_layout_row_end(nk);

        // Second row
        // Split Timer
        nk_style_set_font(nk, &game.ui->fonts()->get("menu_sm")->handle);
        nk_layout_row_static(nk, 35_dp, static_cast<int>(timer_width), 1);
        float split_time = race_manager.splitTimer();

        // show last split time for longer
        int checkpoint_index = race_manager.checkpointIndex();
        lastCheckpointSplitExtension_.update(game.input->timeDelta());
        if (checkpoint_index > 0 && lastCheckpointPassedIndex_ != checkpoint_index) {
            lastCheckpointPassedIndex_ = checkpoint_index;
            // extend the last spit time shown for x seconds
            lastCheckpointSplitExtension_ = 0.6f;
        }
        if (!lastCheckpointSplitExtension_.isZero()) {
            split_time = race_manager.splitTime(checkpoint_index);
        }
        std::string split_str = (split_time < 0 ? "-" : "+") + formatTimeRaceClock(split_time);
        auto split_time_color = nk_rgb(77, 77, 77);
        if (split_time > 0)
            split_time_color = nk_rgb(84, 3, 2);
        else if (split_time < 0)
            split_time_color = nk_rgb(5, 84, 1);
        nk_label_colored(nk, split_str.c_str(), NK_TEXT_RIGHT, split_time_color);
        nk_style_pop_vec2(nk);  // text padding

        nk_group_end(nk);
    }
    nk_style_pop_vec2(nk);  // row spacing
    nk_style_pop_vec2(nk);  // group padding
}

void Hud::drawBoostMeter_(Game &game, struct nk_context *nk, struct nk_rect &bounds) {
    MainController &controller = dynamic_cast<MainController &>(*game.controller);
    const ui::Skin &skin = *game.ui->skin();

    const float boost_meter_height = 180_dp;
    const float boost_meter_width = 30_dp;
    nk_layout_space_push(nk, nk_rect(bounds.w / 2 + bounds.w / 5 - boost_meter_width, bounds.h / 2 - boost_meter_height / 2, boost_meter_width, boost_meter_height));
    nk->style.window.background = nk_rgba(0, 0, 0, 0);
    nk->style.window.fixed_background = nk_style_item_color(nk_rgba(0, 0, 0, 0));
    if (nk_group_begin(nk, "boost", NK_WINDOW_NO_SCROLLBAR)) {
        auto canvas_space = nk_window_get_content_region(nk);
        nk_layout_row_dynamic(nk, boost_meter_height, 1);
        auto canvas = nk_window_get_canvas(nk);
        // note: don't use _dp here, I think
        auto meter_space = nk_rect(canvas_space.x + 5, canvas_space.y + 8, canvas_space.w - 10, canvas_space.h - 8 * 2);

        // background
        nk_draw_nine_slice(canvas, nk_rect(canvas_space.x, canvas_space.y, canvas_space.w, canvas_space.h), &skin.progressNormalBackground, nk_rgba_f(1, 1, 1, 1));
        // meter
        float meter_height = meter_space.h * controller.character->boostMeter();
        nk_fill_rect(canvas, nk_rect(meter_space.x, meter_space.y + (meter_space.h - meter_height), meter_space.w, meter_height), 0, nk_rgba_f(1, 1, 1, 0.7f));

        nk_group_end(nk);
    }
}

void Hud::drawCrosshair_(Game &game, struct nk_context *nk, struct nk_rect &bounds) {
    const float crosshair_size = 30_dp;
    auto canvas = nk_window_get_canvas(nk);
    uint16_t width = static_cast<uint16_t>(crosshairImage_->width());
    uint16_t height = static_cast<uint16_t>(crosshairImage_->height());
    auto img = nk_subimage_id(crosshairImage_->id(), width, height, nk_rect(0, 0, width, height));
    nk_draw_image(canvas, nk_rect(bounds.w / 2 - crosshair_size / 2, bounds.h / 2 - crosshair_size / 2, crosshair_size, crosshair_size), &img, nk_rgb_f(1.0, 1.0, 1.0));
}

void Hud::drawVelocity_(Game &game, struct nk_context *nk, struct nk_rect &bounds) {
    MainController &controller = dynamic_cast<MainController &>(*game.controller);
    const ui::Skin &skin = *game.ui->skin();

    const float speedometer_height = 40_dp;
    const float speedometer_width = 48_dp * 3;
    nk_layout_space_push(nk, nk_rect(bounds.w / 2 - speedometer_width / 2, bounds.h / 2 + bounds.h / 7 + speedometer_height, speedometer_width, speedometer_height));
    nk->style.window.background = nk_rgba_f(0, 0, 0, 0);
    nk->style.window.fixed_background = nk_style_item_color(nk_rgba_f(0, 0, 0, 0));
    nk_style_push_vec2(nk, &nk->style.window.group_padding, nk_vec2(0, 0));
    if (nk_group_begin(nk, "velocity", NK_WINDOW_NO_SCROLLBAR)) {
        auto canvas = nk_window_get_canvas(nk);

        nk_style_set_font(nk, &game.ui->fonts()->get("menu_sm")->handle);
        nk_layout_row_dynamic(nk, speedometer_height, 1);

        struct nk_rect speedometer_bounds;
        nk_widget(&speedometer_bounds, nk);
        nk_draw_nine_slice(canvas, speedometer_bounds, &skin.speedometerBackground, nk_rgba_f(1, 1, 1, 1));
        nk->active->layout->at_y -= nk->active->layout->row.height;  // roll-back position

        float speed = glm::length(controller.character->velocity());
        std::string speed_str = std::format("{} m/s", round(speed));
        nk_label_colored(nk, speed_str.c_str(), NK_TEXT_ALIGN_CENTERED | NK_TEXT_ALIGN_MIDDLE, nk_rgb(28, 28, 28));

        nk_group_end(nk);
    }
    nk_style_pop_vec2(nk);  // group padding
}

void Hud::draw() {
    Game &game = Game::get();
    nk_context *nk = game.ui->context();
    // make window semi transparent
    nk->style.window.background = nk_rgba(0, 0, 0, 0);
    nk->style.window.fixed_background = nk_style_item_color(nk_rgba(0, 0, 0, 0));

    nk_style_push_vec2(nk, &nk->style.window.padding, nk_vec2(0, 0));
    nk_style_set_font(nk, &game.ui->fonts()->get("menu_md")->handle);
    nk->style.text.color = nk_rgb(255, 255, 255);
    if (nk_begin(nk, "hud", nk_rect(0, 0, 100_vw, 100_vh), NK_WINDOW_NO_SCROLLBAR)) {
        nk_layout_space_begin(nk, NK_STATIC, 100_vh, INT_MAX);
        auto bounds = nk_layout_space_bounds(nk);

        // timer
        drawTimer_(game, nk, bounds);
        drawBoostMeter_(game, nk, bounds);
        drawVelocity_(game, nk, bounds);
        drawCrosshair_(game, nk, bounds);

        nk_layout_space_end(nk);
    }
    nk_style_pop_vec2(nk);
    nk_end(nk);
}
