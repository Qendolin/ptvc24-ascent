#pragma once
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <tweeny/tweeny.h>

#include <variant>

#include "Camera.h"
#include "Direct.h"
#include "Entity.h"
#include "Input.h"
#include "Loader/Loader.h"
#include "Physics/Physics.h"
#include "UI/Screen.h"
#include "UI/UI.h"

class Game {
   private:
    float tweenTimer_ = 0.0;
    int tweenTimeStep_ = 0;

    // Called on every game loop iteration
    void loop_();
    // Process user input
    void processInput_();

   public:
    inline static Game *instance = nullptr;

    GLFWwindow *window = nullptr;
    PH::Physics *physics = nullptr;
    NK::Backend *ui = nullptr;
    NK::FontAtlas *fonts = nullptr;
    Input *input = nullptr;

    // Callbacks for laoding / unloading assets
    // Doing it this way makes reloading easy
    std::vector<std::function<void()>> onLoad = {};
    std::vector<std::function<void()>> onUnload = {};

    // Thew viewport size is the window size minus the border and title bar
    glm::ivec2 viewportSize = {1600, 900};
    Camera *camera = nullptr;

    // A quad with dimensions (-1,-1) to (1,1)
    GL::VertexArray *quad = nullptr;
    DirectBuffer *dd = nullptr;

    GL::ShaderPipeline *skyShader = nullptr;
    GL::ShaderPipeline *pbrShader = nullptr;

    Screen *screen = nullptr;
    // All visual instances that need to be rendered.
    // Loaded from the gltf file.
    Asset::Scene scene = {};
    // All entities that need to process game logic
    std::vector<Entity *> entities = {};

    // Initializes the games subsystems like input handling and physics
    Game(GLFWwindow *window);

    // Load assets and such
    void setup();

    // Run the game until the window is closed
    void run();

    // Resize the window's contents, not the window itself.
    void resize(int width, int height);

    template <typename T, typename... Ts>
    void tween(tweeny::tween<T, Ts...> &tween) {
        tween.step(tweenTimeStep_);
    }
};