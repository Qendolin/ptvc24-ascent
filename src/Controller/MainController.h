#pragma once

#include <memory>

#include "GameController.h"

#pragma region ForwardDecl
class MaterialBatchRenderer;
class SkyRenderer;
class Camera;
namespace loader {
class Scene;
}
namespace scene {
class Scene;
}
#pragma endregion

class MainController : public GameController {
   private:
    std::unique_ptr<MaterialBatchRenderer> materialBatchRenderer;
    std::unique_ptr<SkyRenderer> skyRenderer;

    std::unique_ptr<loader::Scene> loaderScene;
    std::unique_ptr<scene::Scene> sceneScene;

    /**
     * Draw the on screen hud (timer, score, etc.)
     */
    void drawHud();

   public:
    MainController(Game &game);

    virtual ~MainController();

    void load() override;

    void unload() override;

    void update() override;

    void render() override;
};
