#pragma once

#include <memory>

#include "../Renderer/ShadowRenderer.h"
#include "AbstractController.h"
#include "RaceManager.h"

#pragma region ForwardDecl
class MaterialBatchRenderer;
class SkyRenderer;
class TerrainRenderer;
class DepthPrepassRenderer;
class MainControllerLoader;
class FadeOverlay;
class ScoreScreen;
class StartScreen;
class PauseScreen;
class Hud;
class CharacterEntity;
class FreeCamEntity;
class Music;
class CSM;
namespace loader {
class SceneData;
class Terrain;
class Environment;

}  // namespace loader
namespace scene {
class Scene;
}
#pragma endregion

class MainController : public AbstractController {
   private:
    std::unique_ptr<MaterialBatchRenderer> materialBatchRenderer;
    std::unique_ptr<SkyRenderer> skyRenderer;
    std::unique_ptr<TerrainRenderer> terrainRenderer;
    std::unique_ptr<ShadowMapRenderer> shadowRenderer;
    std::unique_ptr<DepthPrepassRenderer> depthPrepassRenderer;

    std::unique_ptr<loader::SceneData> sceneData;
    std::unique_ptr<scene::Scene> scene;
    std::unique_ptr<loader::Environment> iblEnv;
    std::unique_ptr<loader::Terrain> terrain;
    std::unique_ptr<MainControllerLoader> loader;
    std::unique_ptr<CSM> csm;

    const std::unique_ptr<ScoreScreen> scoreScreen;
    const std::unique_ptr<StartScreen> startScreen;
    const std::unique_ptr<PauseScreen> pauseScreen;
    const std::unique_ptr<Hud> hud;

    std::unique_ptr<FreeCamEntity> freeCam;

    /**
     * Is called after loading is finished
     */
    void applyLoadResult_();

   public:
    const std::unique_ptr<FadeOverlay> fader;

    RaceManager raceManager;
    CharacterEntity* character = nullptr;

    MainController(Game& game);

    virtual ~MainController();

    void load() override;

    void unload() override;

    void update() override;

    void render() override;

    bool useHdr() override;
};
