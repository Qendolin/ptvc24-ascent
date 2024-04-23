#pragma once

#include <memory>
#include <vector>

#include "Debug/Settings.h"
#include "Settings.h"

#pragma region ForwardDecl
#include "GL/Declarations.h"
class Camera;
class Input;
class DirectBuffer;
class TweenSystem;
class DebugMenu;
class Screen;
struct Window;
class AbstractController;
class FinalizationRenderer;
class BloomRenderer;
class ScoreManager;

namespace ph {
class Physics;
}

namespace ui {
class Backend;
class ImGuiBackend;
}  // namespace ui

namespace loader {
class SceneData;
}

namespace scene {
class Scene;
}
#pragma endregion

class Game {
   private:
    inline static Game *instance_ = nullptr;

    std::unique_ptr<DebugMenu> debugMenu_;
    // the controller which will get activated at the start of the next frame
    std::unique_ptr<AbstractController> queuedController_;
    std::unique_ptr<FinalizationRenderer> finalizationRenderer_;
    std::unique_ptr<BloomRenderer> bloomRenderer_;
    gl::Framebuffer *hdrFramebuffer_;

    // Process user input
    void processInput_();

    // Called every frame before `render_`
    void update_();
    // Called every frame after `update_`
    void render_();

   public:
    // get the game instance singleton
    static Game &get();

    Window &window;
    DebugSettings debugSettings = {};
    std::unique_ptr<ph::Physics> physics;
    std::unique_ptr<ui::Backend> ui;
    std::unique_ptr<ui::ImGuiBackend> imgui;
    std::unique_ptr<Input> input;
    std::unique_ptr<TweenSystem> tween;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<DirectBuffer> directDraw;

    std::unique_ptr<AbstractController> controller;
    std::unique_ptr<ScoreManager> scores;
    SettingsManager settings = SettingsManager("ascent_data/settings.ini");

    // prevent copy
    Game(Game const &) = delete;
    Game &operator=(Game const &) = delete;

    // Initializes the games subsystems like input handling and physics
    Game(Window &window);
    ~Game();

    // Called when the game starts and when assets are reloaded
    void load();

    // Called when the game exits and when assets are reloaded
    void unload();

    // Run the game until the window is closed
    void run();

    // Resize the window's contents, not the window itself.
    void resize(int width, int height);

    /**
     * Queue a controller to be activated next frame.
     */
    template <typename T>
    void queueController() {
        queuedController_ = std::make_unique<T>(*this);
    }
};