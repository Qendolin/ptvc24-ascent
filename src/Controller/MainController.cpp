#include "MainController.h"

#include <algorithm>
#include <chrono>
#include <glm/glm.hpp>

#include "../Camera.h"
#include "../Debug/Direct.h"
#include "../Debug/ImGuiBackend.h"
#include "../Game.h"
#include "../Input.h"
#include "../Loader/Gltf.h"
#include "../Physics/Physics.h"
#include "../Renderer/MaterialBatchRenderer.h"
#include "../Renderer/SkyRenderer.h"
#include "../Scene/Character.h"
#include "../Scene/Entity.h"
#include "../Scene/Objects.h"
#include "../Scene/Scene.h"
#include "../UI/UI.h"
#include "../Utils.h"
#include "../Window.h"

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

MainController::MainController(Game &game) : GameController(game) {}

MainController::~MainController() = default;

void MainController::load() {
    materialBatchRenderer = std::make_unique<MaterialBatchRenderer>();
    skyRenderer = std::make_unique<SkyRenderer>();

    if (sceneData == nullptr) {
        const tinygltf::Model &model = loader::gltf("assets/models/test_course.glb");
        sceneData = std::unique_ptr<loader::SceneData>(loader::scene(model));
        sceneData->physics.create(*game.physics);

        scene::NodeEntityFactory factory;
        scene::registerEntityTypes(factory);
        scene = std::make_unique<scene::Scene>(*sceneData, factory);
        scene->entities.push_back(new CharacterController(scene::SceneRef(*scene), *game.camera));
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

std::string formatTimeMMSS(float total_seconds) {
    std::chrono::duration<float> duration(total_seconds);
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
    auto seconds = duration - minutes;

    return std::format("{:02}:{:02}", round(minutes.count()), round(seconds.count()));
}

void MainController::drawHud() {
    using namespace ui::literals;
    nk_context *nk = game.ui->context();
    // make window transparent
    nk->style.window.background = nk_rgba(0, 0, 0, 0);
    nk->style.window.fixed_background = nk_style_item_color(nk_rgba(0, 0, 0, 0));
    if (nk_begin(nk, "gui", nk_recti(0, 0, (int)100_vw, (int)100_vh), 0)) {
        nk_layout_row_dynamic(nk, 30, 1);
        std::chrono::duration<float> total_seconds(raceManager.timer());
        auto minutes = std::chrono::duration_cast<std::chrono::minutes>(total_seconds);
        auto seconds = total_seconds - minutes;

        std::string time_str = formatTimeMMSS(raceManager.timer());
        nk_label(nk, time_str.c_str(), NK_TEXT_ALIGN_RIGHT);

        nk_layout_row_dynamic(nk, 30, 1);
        std::string penalty_str = "+" + formatTimeMMSS(raceManager.penalty());
        nk_label(nk, penalty_str.c_str(), NK_TEXT_ALIGN_RIGHT);
    }
    nk_end(nk);
}

void MainController::render() {
    drawHud();

    if (game.debugSettings.entity.debugDrawEnabled) {
        for (auto &&ent : scene->entities) ent->debugDraw();
    }

    materialBatchRenderer->render(*game.camera, sceneData->graphics);
    skyRenderer->render(*game.camera);
}
