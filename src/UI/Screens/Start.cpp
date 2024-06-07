#include "Start.h"

#include "../../GL/Texture.h"
#include "../../Game.h"
#include "../../Input.h"
#include "../../Window.h"
#include "../UI.h"

StartScreen::StartScreen() {
    inputPromptsAtlas_ = std::make_unique<loader::Atlas>(
        "assets/textures/input/controls",
        512,
        std::map<std::string, std::string>{
            {"space", "assets/textures/input/Space_Key.png"},
            {"shift", "assets/textures/input/Shift_Key.png"},
            {"w", "assets/textures/input/W_Key.png"},
            {"s", "assets/textures/input/S_Key.png"},
            {"r", "assets/textures/input/R_Key.png"},
            {"esc", "assets/textures/input/Esc_Key.png"},
        });
}

void StartScreen::open() {
    opened_ = true;
    Game::get().input->setMouseMode(Input::MouseMode::Capture);
}

struct nk_image StartScreen::inputSprite_(std::string key) {
    auto rect = inputPromptsAtlas_->rect(key);
    int16_t size = static_cast<int16_t>(inputPromptsAtlas_->size());
    return nk_subimage_id(inputPromptsAtlas_->texture().id(), size, size, nk_rect((int16_t)rect.x, (int16_t)rect.y, (int16_t)rect.w, (int16_t)rect.h));
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

    // transparent background
    nk->style.window.background = nk_rgba(0, 0, 0, 0);
    nk->style.window.fixed_background = nk_style_item_color(nk_rgba(0, 0, 0, 0));
    nk->style.text.color = nk_rgba_f(1, 1, 1, 1);
    if (nk_begin(nk, "start_screen_prompts", {5_vw, 5_vh, 95_vw, 95_vh}, NK_WINDOW_NO_SCROLLBAR)) {
        nk_style_set_font(nk, &game.ui->fonts()->get("menu_sm")->handle);

        float height = 60_dp;
        nk_layout_row_begin(nk, NK_STATIC, height, 4);
        auto w_sprite = inputSprite_("w");
        nk_layout_row_push(nk, height * w_sprite.region[2] / w_sprite.region[3]);
        nk_image(nk, w_sprite);
        auto shift_sprite = inputSprite_("shift");
        nk_layout_row_push(nk, height * shift_sprite.region[2] / shift_sprite.region[3]);
        nk_image(nk, shift_sprite);
        nk_layout_row_push(nk, 20_dp);
        nk_spacer(nk);
        nk_layout_row_push(nk, 120_dp);
        nk_label(nk, "Boost", NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_MIDDLE);

        nk_layout_row_begin(nk, NK_STATIC, height, 4);
        auto s_sprite = inputSprite_("s");
        nk_layout_row_push(nk, height * s_sprite.region[2] / s_sprite.region[3]);
        nk_image(nk, s_sprite);
        auto space_sprite = inputSprite_("space");
        nk_layout_row_push(nk, height * space_sprite.region[2] / space_sprite.region[3]);
        nk_image(nk, space_sprite);
        nk_layout_row_push(nk, 20_dp);
        nk_spacer(nk);
        nk_layout_row_push(nk, 120_dp);
        nk_label(nk, "Break", NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_MIDDLE);

        nk_layout_row_begin(nk, NK_STATIC, height, 3);
        auto r_sprite = inputSprite_("r");
        nk_layout_row_push(nk, height * r_sprite.region[2] / r_sprite.region[3]);
        nk_image(nk, r_sprite);
        nk_layout_row_push(nk, 20_dp);
        nk_spacer(nk);
        nk_layout_row_push(nk, 120_dp);
        nk_label(nk, "Respawn", NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_MIDDLE);

        nk_layout_row_begin(nk, NK_STATIC, height, 3);
        auto esc_sprite = inputSprite_("esc");
        nk_layout_row_push(nk, height * esc_sprite.region[2] / esc_sprite.region[3]);
        nk_image(nk, esc_sprite);
        nk_layout_row_push(nk, 20_dp);
        nk_spacer(nk);
        nk_layout_row_push(nk, 120_dp);
        nk_label(nk, "Pause", NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_MIDDLE);
    }
    nk_end(nk);
}