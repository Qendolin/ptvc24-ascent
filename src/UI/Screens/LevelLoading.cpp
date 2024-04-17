#include "LevelLoading.h"

#include <atomic>
#include <thread>

#include "../../Game.h"
#include "../../Input.h"
#include "../../Loader/Environment.h"
#include "../../Loader/Gltf.h"
#include "../../Utils.h"
#include "../../Window.h"
#include "../UI.h"

class LoaderTask {
   private:
    std::atomic<bool> finished_{false};
    std::thread thread_;
    bool started = false;

    void end_() {
        finished_.store(true);
    }

    void load_() {
        result->gltf = std::make_shared<tinygltf::Model>(
            loader::gltf("assets/models/test_course.glb"));

        result->environment = std::shared_ptr<loader::IblEnv>(
            loader::environment("assets/textures/skybox/kloofendal.iblenv"));
        result->environmentDiffuse = std::shared_ptr<loader::IblEnv>(
            loader::environment("assets/textures/skybox/kloofendal_diffuse.iblenv"));
        result->environmentSpecular = std::shared_ptr<loader::IblEnv>(
            loader::environment("assets/textures/skybox/kloofendal_specular.iblenv"));
        result->iblBrdfLut = std::shared_ptr<loader::FloatImage>(
            loader::floatImage("assets/textures/ibl_brdf_lut.f32"));
        end_();
    }

   public:
    GameLoadingScreen::Data* result = new GameLoadingScreen::Data();

    ~LoaderTask() {
        delete result;
        if (started)
            thread_.detach();
    }

    bool isFinished() {
        return finished_.load();
    }

    void start() {
        if (started)
            return;

        started = true;
        thread_ = std::thread(&LoaderTask::load_, this);
    }
};

GameLoadingScreen::GameLoadingScreen() {
    task_ = std::make_unique<LoaderTask>();
    task_->start();
    startTime_ = Game::get().input->time();
}

GameLoadingScreen::~GameLoadingScreen() {
}

void GameLoadingScreen::draw() {
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

    if (task_->isFinished()) {
        callback(*task_->result);
        task_.reset();
        close();
    }

    nk_end(nk);
}