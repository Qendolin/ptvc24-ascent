#include "MainController.h"

#include <chrono>
#include <glm/glm.hpp>

#include "../Camera.h"
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
        scene->entities.push_back(new CharacterController(*game.camera));
        scene->callEntityInit();

        game.physics->system->OptimizeBroadPhase();
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

void MainController::drawHud() {
    using namespace ui::literals;
    nk_context *nk = game.ui->context();
    // make window transparent
    nk->style.window.background = nk_rgba(0, 0, 0, 0);
    nk->style.window.fixed_background = nk_style_item_color(nk_rgba(0, 0, 0, 0));
    if (nk_begin(nk, "gui", nk_recti(0, 0, (int)100_vw, (int)100_vh), 0)) {
        nk_layout_row_dynamic(nk, 30, 2);
        std::chrono::duration<float> total_seconds(game.input->time());
        auto minutes = std::chrono::duration_cast<std::chrono::minutes>(total_seconds);
        auto seconds = total_seconds - minutes;

        std::string time = std::format("Time: {:02}:{:02}", round(minutes.count()), round(seconds.count()));
        nk_label(nk, time.c_str(), NK_TEXT_ALIGN_LEFT);
    }
    nk_end(nk);
}

void MainController::render() {
    materialBatchRenderer->render(*game.camera, sceneData->graphics);
    skyRenderer->render(*game.camera);
}
