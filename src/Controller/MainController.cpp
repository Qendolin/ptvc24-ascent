#include "MainController.h"

#include <algorithm>
#include <glm/glm.hpp>

#include "../Audio/Assets.h"
#include "../Camera.h"
#include "../Debug/Direct.h"
#include "../Debug/ImGuiBackend.h"
#include "../GL/Framebuffer.h"
#include "../Game.h"
#include "../Input.h"
#include "../Loader/Environment.h"
#include "../Loader/Gltf.h"
#include "../Loader/Terrain.h"
#include "../Loader/Water.h"
#include "../Particles/ParticleSystem.h"
#include "../Physics/Physics.h"
#include "../Renderer/DepthPrepassRenderer.h"
#include "../Renderer/MaterialBatchRenderer.h"
#include "../Renderer/SkyRenderer.h"
#include "../Renderer/TerrainRenderer.h"
#include "../Renderer/WaterRenderer.h"
#include "../Scene/Character.h"
#include "../Scene/Entity.h"
#include "../Scene/FreeCam.h"
#include "../Scene/Objects.h"
#include "../Scene/Scene.h"
#include "../UI/Screens/Fade.h"
#include "../UI/Screens/Hud.h"
#include "../UI/Screens/Pause.h"
#include "../UI/Screens/Score.h"
#include "../UI/Screens/Start.h"
#include "../UI/Skin.h"
#include "../UI/UI.h"
#include "../Util/Log.h"
#include "../Window.h"
#include "MainControllerLoader.h"
#include "MainMenuController.h"

MainController::MainController(Game &game)
    : AbstractController(game),
      scoreScreen(std::make_unique<ScoreScreen>()),
      startScreen(std::make_unique<StartScreen>()),
      pauseScreen(std::make_unique<PauseScreen>()),
      fader(std::make_unique<FadeOverlay>()),
      hud(std::make_unique<Hud>())  //
{
    loader = std::make_unique<MainControllerLoader>();
    freeCam = std::make_unique<FreeCamEntity>(*game.camera);
}

MainController::~MainController() {
    JPH::BodyInterface &physics = game.physics->interface();
    if (sceneData != nullptr) {
        for (loader::PhysicsInstance &instance : sceneData->physics.instances) {
            physics.RemoveBody(instance.id);
            physics.DestroyBody(instance.id);
        }
    }
    if (terrain != nullptr) {
        physics.RemoveBody(terrain->physicsBody()->GetID());
        terrain->destroyPhysicsBody(physics);
    }
}

void MainController::load() {
    LOG_INFO("Started loading");
    loader->load();

    game.particles->loadMaterial(
        "fire", ParticleMaterialParams{
                    .blending = ParticleBlending::AlphaClip,
                    .sprite = "assets/textures/particle/circle.png",
                    .tint = "assets/textures/particle/fire_tint.png",
                    .scale = "assets/textures/particle/fire_scale.png",
                });

    shadowRenderer = std::make_unique<ShadowMapRenderer>();
    terrainRenderer = std::make_unique<TerrainRenderer>();
    depthPrepassRenderer = std::make_unique<DepthPrepassRenderer>();
    csm = std::make_unique<CSM>(2048, 1.0f / 30.0f);

    game.audio->assets->bgm.pause();
    game.audio->assets->bgm.seek(0);
}

bool MainController::useHdr() {
    return !loader->isLoading();
}

void MainController::applyLoadResult_() {
    LOG_INFO("Finished loading");
    MainControllerLoader::Data data = loader->result();
    JPH::BodyInterface &physics = game.physics->interface();

    materialBatchRenderer = std::make_unique<MaterialBatchRenderer>();
    iblEnv = std::make_unique<loader::Environment>(*data.environment, *data.environmentDiffuse, *data.environmentSpecular, *data.iblBrdfLut);
    skyRenderer = std::make_unique<SkyRenderer>();
    waterTRenderer = std::make_unique<WaterRenderer>();
    if (terrain != nullptr) {
        physics.RemoveBody(terrain->physicsBody()->GetID());
        terrain->destroyPhysicsBody(physics);
    }
    terrain = std::make_unique<loader::Terrain>(*data.terrain, 4096.0f, 1200.0f, glm::vec3(0), 20);
    terrain->createPhysicsBody(physics, glm::vec3(0.0, 1.0, 0.0));
    water = std::make_unique<loader::Water>(*data.water, 4096.0f * 4, 40.0f, glm::vec3(0, 40, 0), 40);
    physics.AddBody(terrain->physicsBody()->GetID(), JPH::EActivation::DontActivate);

    if (!data.gltf) return;

    // Should only ever be called once per instance

    LOG_INFO("Creating scene from gltf data");
    fader->fade(1.0f, 0.0f, 0.3f);
    startScreen->open();

    sceneData = std::unique_ptr<loader::SceneData>(loader::scene(*data.gltf));
    for (loader::PhysicsInstance &instance : sceneData->physics.instances) {
        JPH::BodyID id = physics.CreateAndAddBody(instance.settings, JPH::EActivation::DontActivate);
        if (!instance.id.IsInvalid()) PANIC("Instance already has a physics body id");
        instance.id = id;
    }

    scene::NodeEntityFactory factory;
    scene::registerEntityTypes(factory);
    scene = std::make_unique<scene::Scene>(*sceneData, factory);

    auto &characterNode = scene->createGenericNode("player");
    character = scene::SceneRef(*scene).create<CharacterEntity>(scene::NodeRef(*scene, characterNode.index), *game.camera);

    scene->callEntityInit();

    // Doing it this way is sooo stupid
    // TODO: clean this up
    characterNode.physics = scene->physics.size();
    scene->physics.emplace_back(scene::Physics{.body = character->body()});
    scene->nodesByBodyID[character->body()] = characterNode.index;
    // add both for better hit detection
    scene->nodesByBodyID[character->kinematicBody()] = characterNode.index;
    characterNode.entity = scene->entities.size() - 1;

    scene->createPhysicsNode("terrain", scene::Physics{.body = terrain->physicsBody()->GetID()});
    game.physics->system->OptimizeBroadPhase();

    scene::SceneRef scene_ref(*scene);
    scene::NodeRef first_checkpoint = scene_ref.find(scene_ref.root(), [](scene::NodeRef &node) {
        return node.prop<bool>("is_first", false);
    });
    scene::NodeRef player_spawn = scene_ref.find(scene_ref.root(), "PlayerSpawn");
    RaceManager::RespawnPoint spawn = {
        .transform = player_spawn.transform().matrix(),
        .speed = player_spawn.prop("speed", 5.0f),
        .boostMeter = 1.0,
    };
    raceManager = RaceManager(character, data.gltf->scenes[data.gltf->defaultScene].name, spawn);
    raceManager.loadCheckpoints(first_checkpoint.entity<CheckpointEntity>());

    character->respawn();
    game.audio->assets->bgm.play();
}

void MainController::unload() {
    game.audio->assets->bgm.pause();
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
        if (pauseScreen->closed())
            pauseScreen->open();
    }

    if (pauseScreen->resetFlag()) {
        switch (pauseScreen->action) {
            case PauseScreen::Action::Exit:
                game.queueController<MainMenuController>();
                break;
            case PauseScreen::Action::Respawn:
                character->respawn();
                break;
        }
    }

    // dont update the game while a screen is up
    if (pauseScreen->opened() || startScreen->opened() || scoreScreen->opened()) {
        return;
    }

    float time_delta = game.input->timeDelta();
    raceManager.update(time_delta);

    // Free Cam
    if (game.debugSettings.freeCam) {
        freeCam->update(time_delta);
    }
    character->enabled = !game.debugSettings.freeCam;

    // Update and step physics
    game.physics->update(time_delta);
    if (game.physics->isNextStepDue()) {
        scene->callEntityPrePhysicsUpdate();
        game.physics->step();
        scene->callEntityPostPhysicsUpdate();
    }

    // Update entities
    scene->callEntityUpdate(time_delta);

    game.particles->update(time_delta);

    if (raceManager.hasEnded() && !scoreScreen->opened()) {
        ScoreEntry score = raceManager.score();
        game.scores->add(score);
        game.scores->save();
        scoreScreen->open(score);
        character->terminate();
    }
}

void MainController::render() {
    // still loading
    if (loader->isLoading()) {
        loader->draw();
        return;
    }

    fader->draw();

    if (scoreScreen->opened()) {
        scoreScreen->draw();
        if (scoreScreen->resetFlag()) {
            game.queueController<MainMenuController>();
        }
    } else if (pauseScreen->opened()) {
        pauseScreen->draw();
    } else if (startScreen->opened()) {
        startScreen->draw();
    } else {
        hud->draw();
    }

    game.camera->updateViewMatrix();

    if (game.debugSettings.entity.debugDrawEnabled) {
        for (auto &&ent : scene->entities) ent->debugDraw();
    }

    if (csm->update(*game.camera, game.debugSettings.rendering.sun.direction(), game.input->timeDelta())) {
        shadowRenderer->render(*csm, *game.camera, sceneData->graphics, *terrain);
    }

    game.hdrFramebuffer().bind(GL_DRAW_FRAMEBUFFER);
    // Depth prepass
    // Does not work! Because model matrix update is unsynchronized.
    // game.hdrFramebuffer().bindTargets({});
    // depthPrepassRenderer->render(*game.camera, sceneData->graphics, *terrain);

    game.hdrFramebuffer().bindTargets({0, 1});
    terrainRenderer->render(*game.camera, *terrain, *csm, *iblEnv, game.debugSettings.rendering.sun);
    materialBatchRenderer->render(*game.camera, sceneData->graphics, *csm, *iblEnv, game.debugSettings.rendering.sun);
    waterTRenderer->render(*game.camera, *water, *iblEnv, game.debugSettings.rendering.sun, game.hdrFramebuffer().getTexture(GL_DEPTH_ATTACHMENT));
    game.hdrFramebuffer().bindTargets({0});
    game.particles->draw(*game.camera);
    skyRenderer->render(*game.camera, *iblEnv);
}
