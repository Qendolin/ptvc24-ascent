#pragma once

#include <functional>
#include <memory>
#include <vector>

#pragma region ForwardDecl
#include "GL/Declarations.h"
class Camera;
class Input;
class DirectBuffer;
class TweenSystem;
class DebugMenu;
class Screen;
struct Window;

namespace ph {
class Physics;
}

namespace ui {
class Backend;
class ImGuiBackend;
}  // namespace ui

namespace loader {
class Scene;
}

namespace scene {
class Scene;
}
#pragma endregion

class Game {
   private:
    inline static Game *instance_ = nullptr;

    std::unique_ptr<DebugMenu> debugMenu_;
    std::unique_ptr<DirectBuffer> directDraw_;

    // Called on every game loop iteration
    void loop_();
    // Process user input
    void processInput_();

   public:
    // get the game instance singleton
    static Game &get();

    Window &window;
    std::unique_ptr<ph::Physics> physics;
    std::unique_ptr<ui::Backend> ui;
    std::unique_ptr<ui::ImGuiBackend> imgui;
    std::unique_ptr<Input> input;
    std::unique_ptr<TweenSystem> tween;

    // Callbacks for laoding / unloading assets
    // Doing it this way makes reloading easy
    std::vector<std::function<void()>> onLoad = {};
    std::vector<std::function<void()>> onUnload = {};

    std::unique_ptr<Camera> camera;

    // A quad with dimensions (-1,-1) to (1,1)
    gl::VertexArray *quad = nullptr;

    gl::ShaderPipeline *skyShader = nullptr;
    gl::ShaderPipeline *pbrShader = nullptr;

    Screen *screen = nullptr;

    // All the stuff loaded from the gltf file (graphics and physics objects).
    loader::Scene *scene = nullptr;
    scene::Scene *entityScene = nullptr;

    // prevent copy
    Game(Game const &) = delete;
    Game &operator=(Game const &) = delete;

    // Initializes the games subsystems like input handling and physics
    Game(Window &window);
    ~Game();

    // Load assets and such
    void setup();

    // Run the game until the window is closed
    void run();

    // Resize the window's contents, not the window itself.
    void resize(int width, int height);
};