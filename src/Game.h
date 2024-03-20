#pragma once
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Camera.h"
#include "Direct.h"
#include "Entity.h"
#include "Input.h"
#include "Loader.h"
#include "Physics/Physics.h"
#include "UI/UI.h"

class Game {
   private:
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

    // Callbacks for laoding / unloading assets
    // Doing it this way makes reloading easy
    std::vector<std::function<void()>> onLoad = {};
    std::vector<std::function<void()>> onUnload = {};

    Input *input = nullptr;
    Camera *camera = nullptr;

    // A quad with dimensions (-1,-1) to (1,1)
    GL::VertexArray *quad = nullptr;
    DirectBuffer *dd = nullptr;

    GL::ShaderPipeline *skyShader = nullptr;
    GL::ShaderPipeline *pbrShader = nullptr;

    // All visual instances that need to be rendered.
    // Loaded from the gltf file.
    std::vector<Scene::Instance> scene = {};
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