#pragma once

#include <memory>

#include "AbstractController.h"
#include "RaceManager.h"

#pragma region ForwardDecl
class MaterialBatchRenderer;
class SkyRenderer;
class MainControllerLoader;
class FadeOverlay;
class ScoreScreen;
class StartScreen;
namespace loader {
class SceneData;
}
namespace scene {
class Scene;
}
#pragma endregion

class MainController : public AbstractController {
   private:
    std::unique_ptr<MaterialBatchRenderer> materialBatchRenderer;
    std::unique_ptr<SkyRenderer> skyRenderer;

    std::unique_ptr<loader::SceneData> sceneData;
    std::unique_ptr<scene::Scene> scene;
    std::unique_ptr<MainControllerLoader> loader;

    std::unique_ptr<ScoreScreen> scoreScreen;
    std::unique_ptr<StartScreen> startScreen;

    /**
     * Draw the on screen hud (timer, score, etc.)
     */
    void drawHud_();

    /**
     * Is called after loading is finished
     */
    void applyLoadResult_();

   public:
    std::unique_ptr<FadeOverlay> fader;

    RaceManager raceManager;

    MainController(Game &game);

    virtual ~MainController();

    void load() override;

    void unload() override;

    void update() override;

    void render() override;
};