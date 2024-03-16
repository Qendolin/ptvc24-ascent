#pragma once
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Camera.h"
#include "Direct.h"
#include "Input.h"
#include "Loader.h"
#include "Entity.h"
#include "Physics/Physics.h"

class Game {
   private:
    void loop_();
    void processInput_();

   public:
    inline static Game *instance;

    GLFWwindow *window = nullptr;
    PH::Physics *physics = nullptr;

    // Callbacks for laoding / unloading assets
    // Doing it this way makes reloading easy
    std::vector<std::function<void()>> onLoad = {};
    std::vector<std::function<void()>> onUnload = {};

    bool mouseCaptured = false;
    Input *input = nullptr;
    Camera *camera = nullptr;

    GL::VertexArray *quad = nullptr;
    DirectBuffer *dd = nullptr;

    GL::ShaderPipeline *skyShader = nullptr;
    GL::ShaderPipeline *pbrShader = nullptr;

    std::vector<Scene::Instance> scene = {};
	std::vector<Entity*> entities = {};
    JPH::Character *characterBody = nullptr;

    Game(GLFWwindow *window);

    void setup();

    void run();
};