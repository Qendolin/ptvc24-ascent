#include "MainController.h"

#include <algorithm>
#include <glm/glm.hpp>

#include "../Camera.h"
#include "../Debug/Direct.h"
#include "../Debug/ImGuiBackend.h"
#include "../Game.h"
#include "../Input.h"
#include "../Loader/Environment.h"
#include "../Loader/Gltf.h"
#include "../Physics/Physics.h"
#include "../Renderer/MaterialBatchRenderer.h"
#include "../Renderer/SkyRenderer.h"
#include "../Scene/Character.h"
#include "../Scene/Entity.h"
#include "../Scene/Objects.h"
#include "../Scene/Scene.h"
#include "../UI/Screens/Fade.h"
#include "../UI/UI.h"
#include "../Util/Log.h"
#include "../Window.h"
#include "MainControllerLoader.h"

template <typename T>
int32_t indexOf(const std::vector<T> &vec, const T elem) {
    auto it = std::find(vec.cbegin(), vec.cend(), elem);
    if (it == vec.end()) {
        return -1;
    }
    return static_cast<int32_t>(std::distance(vec.cbegin(), it));
}

void RaceManager::onCheckpointEntered(CheckpointEntity *checkpoint) {
    int32_t index = indexOf(checkpoints, checkpoint);
    LOG_DEBUG("Entered checkpoint '" + std::to_string(index) + "'");

    // first checkpoint
    if (startTime < 0) {
        startTime = Game::get().input->time();
        lastPassedCheckpoint = index;
        return;
    }

    if (index > lastPassedCheckpoint) {
        int32_t skipped = std::max(index - lastPassedCheckpoint - 1, 0);
        penaltyTime += skipped * 5;
        lastPassedCheckpoint = index;
    }
}

void RaceManager::loadCheckpoints(CheckpointEntity *start) {
    checkpoints.clear();
    checkpoints.push_back(start);
    CheckpointEntity *current = start;
    while (current->hasNextCheckpoint()) {
        CheckpointEntity *next = current->nextCheckpoint();
        checkpoints.push_back(next);
        current = next;
    }
}

CheckpointEntity *RaceManager::getLastCheckpoint() {
    if (lastPassedCheckpoint < 0) return nullptr;
    return checkpoints[lastPassedCheckpoint];
}

float RaceManager::timer() {
    if (startTime < 0) return 0;
    return static_cast<float>(Game::get().input->time() - startTime);
}

MainController::MainController(Game &game) : AbstractController(game) {
    loader = std::make_unique<MainControllerLoader>();
    fader = std::make_unique<FadeOverlay>();
}

MainController::~MainController() = default;

void MainController::load() {
    LOG_INFO("Started loading");
    loader->load();
}

void MainController::applyLoadResult_() {
    LOG_INFO("Finished loading");
    MainControllerLoader::Data data = loader->result();
    materialBatchRenderer = std::make_unique<MaterialBatchRenderer>(data.environmentDiffuse, data.environmentSpecular, data.iblBrdfLut);
    skyRenderer = std::make_unique<SkyRenderer>(data.environment);

    if (data.gltf) {
        LOG_INFO("Creating scene from gltf data");
        fader->fade(1.0f, 0.0f, 0.3f);

        sceneData = std::unique_ptr<loader::SceneData>(loader::scene(*data.gltf));
        sceneData->physics.create(*game.physics);

        scene::NodeEntityFactory factory;
        scene::registerEntityTypes(factory);
        scene = std::make_unique<scene::Scene>(*sceneData, factory);
        auto character = new CharacterController(scene::SceneRef(*scene), *game.camera);
        scene->entities.push_back(character);
        scene->callEntityInit();

        game.physics->system->OptimizeBroadPhase();

        scene::SceneRef sceneRef(*scene);
        scene::NodeRef first_Checkpoint = sceneRef.find(sceneRef.root(), [](scene::NodeRef &node) {
            return node.prop<bool>("is_first", false);
        });
        raceManager.loadCheckpoints(first_Checkpoint.entity<CheckpointEntity>());
    }
}

void MainController::unload() {
}

void MainController::update() {
    bool can_capture_mouse = !game.imgui->shouldShowCursor();
    // Capture mouse
    if (game.input->isMousePress(GLFW_MOUSE_BUTTON_LEFT) && game.input->isMouseReleased() && can_capture_mouse) {
        game.input->captureMouse();
    }
    // Release mouse
    if (game.input->isKeyPress(GLFW_KEY_ESCAPE) && game.input->isMouseCaptured()) {
        game.input->releaseMouse();
    }

    // still loading
    loader->update();
    if (loader->isLoading()) {
        return;
    } else if (loader->isDone()) {
        applyLoadResult_();
    }

    // Update and step physics
    game.physics->update(game.input->timeDelta());
    if (game.physics->isNextStepDue()) {
        scene->callEntityPrePhysicsUpdate();
        game.physics->step();
        scene->callEntityPostPhysicsUpdate();
    }

    // Update entities
    scene->callEntityUpdate();
}

std::string formatTimeRaceClock(float total_seconds) {
    std::chrono::duration<float> duration(total_seconds);
    float minutes = std::floor(total_seconds / 60);
    float seconds = std::floor(total_seconds - minutes * 60);
    float millis = std::floor(1000 * (total_seconds - minutes * 60 - seconds));

    return std::format("{:02}:{:02}.{:03}", round(minutes), round(seconds), round(millis));
}

void MainController::drawHud_() {
    using namespace ui::literals;
    nk_context *nk = game.ui->context();
    // make window semi transparent
    nk->style.window.background = nk_rgba(0, 0, 0, 120);
    nk->style.window.fixed_background = nk_style_item_color(nk_rgba(0, 0, 0, 120));
    if (nk_begin(nk, "timer", nk_rect(100_vw - 180_dp, 0, 180_dp, 90_dp), 0)) {
        nk_style_push_color(nk, &nk->style.text.color, nk_rgb(255, 255, 255));
        nk_layout_row_dynamic(nk, 30_dp, 1);

        std::string time_str = formatTimeRaceClock(raceManager.timer());
        nk_label(nk, time_str.c_str(), NK_TEXT_ALIGN_RIGHT);

        nk_layout_row_dynamic(nk, 30_dp, 1);
        std::string penalty_str = "+" + formatTimeRaceClock(raceManager.penalty());
        nk_label(nk, penalty_str.c_str(), NK_TEXT_ALIGN_RIGHT);
        nk_style_pop_color(nk);
    }
    nk_end(nk);
}

void MainController::render() {
    // still loading
    if (loader->isLoading()) {
        loader->draw();
        return;
    }

    drawHud_();
    fader->draw();

    if (game.debugSettings.entity.debugDrawEnabled) {
        for (auto &&ent : scene->entities) ent->debugDraw();
    }

    materialBatchRenderer->render(*game.camera, sceneData->graphics);
    skyRenderer->render(*game.camera);
}
