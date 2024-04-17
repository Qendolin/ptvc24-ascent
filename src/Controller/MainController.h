#pragma once

#include <memory>
#include <vector>

#include "AbstractController.h"

#pragma region ForwardDecl
class MaterialBatchRenderer;
class SkyRenderer;
class Camera;
class CheckpointEntity;
class MainControllerLoader;
class FadeOverlay;
namespace loader {
class SceneData;
}
namespace scene {
class Scene;
}
#pragma endregion

// Handles the logic of measuring score and checkpoints
class RaceManager {
    int lastPassedCheckpoint = -1;
    double startTime = -1.0;
    float penaltyTime = 0.0;
    std::vector<CheckpointEntity *> checkpoints;

   public:
    void onCheckpointEntered(CheckpointEntity *checkpoint);

    void loadCheckpoints(CheckpointEntity *start);

    CheckpointEntity *getLastCheckpoint();

    // @return timer time in seconds
    float timer();

    // @return penalty time in seconds
    float penalty() {
        return penaltyTime;
    }
};

class MainController : public AbstractController {
   private:
    std::unique_ptr<MaterialBatchRenderer> materialBatchRenderer;
    std::unique_ptr<SkyRenderer> skyRenderer;

    std::unique_ptr<loader::SceneData> sceneData;
    std::unique_ptr<scene::Scene> scene;
    std::unique_ptr<MainControllerLoader> loader;

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
