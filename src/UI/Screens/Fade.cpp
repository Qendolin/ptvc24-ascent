#include "Fade.h"

#include "../../Game.h"
#include "../../Input.h"
#include "../../Loader/Environment.h"
#include "../../Loader/Gltf.h"
#include "../../Util/Log.h"
#include "../../Util/Task.h"
#include "../../Window.h"
#include "../UI.h"

FadeOverlay::FadeOverlay() = default;

FadeOverlay::~FadeOverlay() = default;

void FadeOverlay::fade(float from, float to, float duration) {
    from_ = from;
    to_ = to;
    duration_ = duration;
    startTime_ = Game::get().input->time();
}

void FadeOverlay::draw() {
    using namespace ui::literals;

    Game& game = Game::get();
    nk_context* nk = game.ui->context();

    double time = game.input->time();
    float progress = (float)std::min(std::max((time - startTime_) / duration_, 0.0), 1.0);
    float alpha = from_ * (1.0f - progress) + to_ * progress;

    // transparent background
    nk->style.window.background = nk_rgba_f(0, 0, 0, alpha);
    nk->style.window.fixed_background = nk_style_item_color(nk_rgba_f(0, 0, 0, alpha));

    nk_begin(nk, "fade_overlay", {0_vw, 0_vh, 100_vw, 100_vh}, NK_WINDOW_NO_SCROLLBAR);
    nk_end(nk);
}