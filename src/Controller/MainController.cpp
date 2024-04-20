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
#include "../UI/Screens/Pause.h"
#include "../UI/Screens/Score.h"
#include "../UI/Screens/Start.h"
#include "../UI/UI.h"
#include "../Util/Format.h"
#include "../Util/Log.h"
#include "../Window.h"
#include "MainControllerLoader.h"
#include "MainMenuController.h"

MainController::MainController(Game &game) : AbstractController(game) {
    loader = std::make_unique<MainControllerLoader>();
    fader = std::make_unique<FadeOverlay>();
}

MainController::~MainController() {
    if (sceneData != nullptr) {
        JPH::BodyInterface &physics = game.physics->interface();
        for (loader::PhysicsInstance &instance : sceneData->physics.instances) {
            physics.RemoveBody(instance.id);
            physics.DestroyBody(instance.id);
        }
    }
}

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
        startScreen = std::make_unique<StartScreen>();

        sceneData = std::unique_ptr<loader::SceneData>(loader::scene(*data.gltf));
        JPH::BodyInterface &physics = game.physics->interface();
        for (loader::PhysicsInstance &instance : sceneData->physics.instances) {
            JPH::BodyID id = physics.CreateAndAddBody(instance.settings, JPH::EActivation::DontActivate);
            if (!instance.id.IsInvalid()) PANIC("Instance already has a physics body id");
            instance.id = id;
        }

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
        raceManager = RaceManager(data.gltf->scenes[data.gltf->defaultScene].name);
        raceManager.loadCheckpoints(first_Checkpoint.entity<CheckpointEntity>());
    }
}

void MainController::unload() {
}

void MainController::update() {
    // still loading
    loader->update();
    if (loader->isLoading()) {
        return;
    } else if (loader->isDone()) {
        applyLoadResult_();
    }

    // pausing
    if (!game.input->isWindowFocused() || (game.input->isMouseReleased() && game.input->mouseMode() == Input::MouseMode::Capture)) {
        if (pauseScreen == nullptr)
            pauseScreen = std::make_unique<PauseScreen>();
    }
    if (pauseScreen && pauseScreen->isClosed())
        pauseScreen = nullptr;

    // dont update the game while a screen is up
    if (pauseScreen != nullptr || startScreen != nullptr || scoreScreen != nullptr) {
        return;
    }

    // Update and step physics
    game.physics->update(game.input->timeDelta());
    if (game.physics->isNextStepDue()) {
        scene->callEntityPrePhysicsUpdate();
        game.physics->step();
        scene->callEntityPostPhysicsUpdate();
    }

    // Update entities
    scene->callEntityUpdate(game.input->timeDelta());

    if (raceManager.hasEnded() && scoreScreen == nullptr) {
        ScoreEntry score = raceManager.score();
        game.scores->add(score);
        game.scores->save();
        scoreScreen = std::make_unique<ScoreScreen>(score);
    }
}

void MainController::drawHud_() {
    using namespace ui::literals;
    nk_context *nk = game.ui->context();
    // make window semi transparent
    nk->style.window.background = nk_rgba(0, 0, 0, 0);
    nk->style.window.fixed_background = nk_style_item_color(nk_rgba(0, 0, 0, 0));

    nk_style_push_vec2(nk, &nk->style.window.padding, nk_vec2(0, 0));
    nk_style_set_font(nk, &game.ui->fonts()->get("menu_md")->handle);
    nk->style.text.color = nk_rgb(255, 255, 255);
    if (nk_begin(nk, "hud", nk_rect(0, 0, 100_vw, 100_vh), NK_WINDOW_NO_SCROLLBAR)) {
        nk_layout_space_begin(nk, NK_STATIC, 90_dp, 1);
        auto bounds = nk_layout_space_bounds(nk);
        nk_layout_space_push(nk, nk_rect(bounds.w - 180_dp, 0, 180_dp, bounds.h));

        nk->style.window.background = nk_rgba(0, 0, 0, 120);
        nk->style.window.fixed_background = nk_style_item_color(nk_rgba(0, 0, 0, 120));
        nk_style_push_vec2(nk, &nk->style.window.group_padding, nk_vec2(14, 4));
        if (nk_group_begin(nk, "timer", NK_WINDOW_NO_SCROLLBAR)) {
            nk_layout_row_dynamic(nk, 30_dp, 1);

            std::string time_str = formatTimeRaceClock(raceManager.timer());
            nk_label(nk, time_str.c_str(), NK_TEXT_ALIGN_RIGHT);

            nk_style_set_font(nk, &game.ui->fonts()->get("menu_sm")->handle);
            nk_layout_row_dynamic(nk, 30_dp, 1);
            float split_time = raceManager.splitTimer();
            std::string split_str = (split_time < 0 ? "-" : "+") + formatTimeRaceClock(split_time);
            nk_label(nk, split_str.c_str(), NK_TEXT_ALIGN_RIGHT);

            nk_group_end(nk);
        }
        nk_style_pop_vec2(nk);
        nk_layout_space_end(nk);
    }
    nk_style_pop_vec2(nk);
    nk_end(nk);
}

void MainController::render() {
    // still loading
    if (loader->isLoading()) {
        loader->draw();
        return;
    }

    if (scoreScreen) {
        scoreScreen->draw();
        if (scoreScreen->isClosed()) {
            game.queueController<MainMenuController>();
            scoreScreen = nullptr;
        }
    } else if (startScreen) {
        startScreen->draw();
        if (startScreen->isClosed()) {
            startScreen = nullptr;
            // TODO: start flying
        }
    } else if (pauseScreen) {
        pauseScreen->draw();
    } else {
        drawHud_();
    }

    fader->draw();

    game.camera->updateViewMatrix();

    if (game.debugSettings.entity.debugDrawEnabled) {
        for (auto &&ent : scene->entities) ent->debugDraw();
    }

    materialBatchRenderer->render(*game.camera, sceneData->graphics);
    skyRenderer->render(*game.camera);
}
