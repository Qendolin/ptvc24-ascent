#include "Loading.h"

#include "../../Game.h"
#include "../../Input.h"
#include "../../Loader/Environment.h"
#include "../../Loader/Gltf.h"
#include "../../Util/Log.h"
#include "../../Util/Task.h"
#include "../../Window.h"
#include "../UI.h"

LoadingScreen::LoadingScreen(TaskCompletionView& task) : task(task) {
    startTime_ = Game::get().input->time();
}

LoadingScreen::~LoadingScreen() {
}

void LoadingScreen::draw() {
    using namespace ui::literals;

    Game& game = Game::get();
    nk_context* nk = game.ui->context();

    // transparent background
    nk->style.window.background = nk_rgba(0, 0, 0, 0);
    nk->style.window.fixed_background = nk_style_item_color(nk_rgba(0, 0, 0, 0));

    if (nk_begin(nk, "level_loading", {15_vw, 80_vh, 70_vw, 20_vh}, 0)) {
        nk_layout_row_dynamic(nk, 30_dp, 1);
        int time = (int)std::round(Game::get().input->time() - startTime_);
        std::string text = "Loading Level" + std::string(time, '.');
        nk_label(nk, text.c_str(), NK_TEXT_ALIGN_LEFT);
    }

    if (task.isFinished()) {
        close();
    }

    nk_end(nk);
}