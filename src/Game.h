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
#include "Loader/Gltf.h"
#include "Loader/Loader.h"
#include "Physics/Physics.h"
#include "UI/Screen.h"
#include "UI/UI.h"

// References:
// https://mobius3.github.io/tweeny/
//
// The "tweeny" tweens need to be updated in discrete time steps.
// This game uses one milliseconds as the smallest unit. (1000 tween units = 1 second)
// The purpose of this class is to aggregate frame times and calculate the `step` value accordingly.
// Examples using the notation: `(carry, step) > time_delta > (next_carry, next_step)`
// 1: `(0.0ms, 0) > 0.4ms > (0.4ms, 0) > 0.4ms > (0.8ms, 0) > 0.4ms > (0.2ms, 1) > 0.4ms > (0.6ms, 0)`
// 2: `(0.0ms, 0) > 12.5ms > (0.5ms, 12) > 16.0ms > (0.5ms, 16) > 10.5ms > (0.0ms, 10)`
class TweenSystem {
   private:
    // the carry aggregates sub-millisecond times until a full millisecond is reached.
    float carry_ = 0.0;
    // the step stores the amount of tweeny time units that each tween should be advanced for this frame.
    int step_ = 0;

   public:
    TweenSystem(){};
    ~TweenSystem(){};

    void update(float time_delta) {
        carry_ += time_delta * 1000.0;
        step_ = (int)floor(carry_);
        carry_ -= step_;
    }

    // Takes a tween and advances it
    template <typename T, typename... Ts>
    void step(tweeny::tween<T, Ts...> &tween) const {
        tween.step(step_);
    }
};

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

    ~Game();

    GLFWwindow *window = nullptr;
    PH::Physics *physics = nullptr;
    UI::Backend *ui = nullptr;
    Input *input = nullptr;
    TweenSystem tween = TweenSystem();

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

    // All the stuff loaded from the gltf file (graphics and physics objects).
    Asset::Scene *scene = nullptr;
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
};