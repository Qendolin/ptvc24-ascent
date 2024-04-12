#include "Game.h"

#include <chrono>
#include <glm/glm.hpp>

#include "Camera.h"
#include "Debug/DebugMenu.h"
#include "Debug/Direct.h"
#include "Debug/ImGuiBackend.h"
#include "GL/Geometry.h"
#include "GL/Shader.h"
#include "GL/StateManager.h"
#include "GL/Texture.h"
#include "Input.h"
#include "Loader/Gltf.h"
#include "Physics/Physics.h"
#include "Physics/Shapes.h"
#include "Scene/Character.h"
#include "Scene/Objects/Checkpoint.h"
#include "Scene/Objects/TestObstacle.h"
#include "Tween.h"
#include "UI/Renderer.h"
#include "UI/Screens/MainMenu.h"
#include "UI/Skin.h"
#include "UI/UI.h"
#include "Utils.h"
#include "Window.h"

gl::VertexArray *createQuad() {
    gl::Buffer *vbo = new gl::Buffer();
    vbo->setDebugLabel("generic_quad/vbo");
    glm::vec2 quad_verts[] = {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}};
    vbo->allocate(&quad_verts, sizeof(quad_verts), 0);

    gl::VertexArray *quad = new gl::VertexArray();
    quad->setDebugLabel("generic_quad/vao");
    quad->layout(0, 0, 2, GL_FLOAT, false, 0);
    quad->bindBuffer(0, *vbo, 0, 2 * 4);
    quad->own(vbo);
    return quad;
}

Game &Game::get() {
    if (instance_ == nullptr) PANIC("No instance");
    return *instance_;
}

Game::Game(Window &window)
    : window(window) {
    instance_ = this;
    tween = std::make_unique<TweenSystem>();
    debugMenu_ = std::make_unique<DebugMenu>();
    input = std::make_unique<Input>(window);
    glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode, int action, int mods) {
        Game::get().input->onKey(window, key, scancode, action, mods);
    });
    glfwSetCursorPosCallback(window, [](GLFWwindow *window, double x, double y) {
        Game::get().input->onCursorPos(window, x, y);
    });
    glfwSetMouseButtonCallback(window, [](GLFWwindow *window, int button, int action, int mods) {
        Game::get().input->onMouseButton(window, button, action, mods);
    });
    glfwSetScrollCallback(window, [](GLFWwindow *window, double dx, double dy) {
        Game::get().input->onScroll(window, dx, dy);
    });
    glfwSetCharCallback(window, [](GLFWwindow *window, unsigned int codepoint) {
        Game::get().input->onChar(window, codepoint);
    });
    glfwSetWindowFocusCallback(window, [](GLFWwindow *window, int focused) {
        Game::get().input->invalidate();
    });

    // Create camera with a 90° vertical FOV
    camera = std::make_unique<Camera>(glm::radians(90.0f), window.size, 0.1f, glm::vec3{0, 1, 1}, glm::vec3{});

    // window / viewport size
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int width, int height) {
        Game::get().resize(width, height);
    });
    int vp_width, vp_height;
    glfwGetFramebufferSize(window, &vp_width, &vp_height);
    Game::resize(vp_width, vp_height);

    physics = std::make_unique<ph::Physics>();
    physics->setDebugDrawEnabled(true);
}

Game::~Game() {
    for (const auto &callback : onUnload) {
        callback();
    }

    delete quad;

    delete skyShader;
    delete pbrShader;

    // delete entities before deleting the physics
    delete entityScene;
    delete scene;

    delete screen;
}

void Game::resize(int width, int height) {
    window.size = glm::ivec2(width, height);

    if (camera != nullptr)
        camera->setViewportSize(window.size);

    float x_scale, y_scale;
    glfwGetWindowContentScale(window, &x_scale, &y_scale);
    ui::set_scale(width, height, x_scale);

    if (ui != nullptr)
        ui->setViewport(window.size.x, window.size.y);

    if (imgui != nullptr)
        imgui->setViewport(window.size.x, window.size.y);
}

void Game::setup() {
    quad = createQuad();

    screen = new MainMenuScreen();

    // defer loading assets
    onLoad.push_back([this]() {
        skyShader = new gl::ShaderPipeline(
            {new gl::ShaderProgram("assets/shaders/sky.vert"),
             new gl::ShaderProgram("assets/shaders/sky.frag")});
        skyShader->setDebugLabel("sky_shader");
        pbrShader = new gl::ShaderPipeline(
            {new gl::ShaderProgram("assets/shaders/test.vert"),
             new gl::ShaderProgram("assets/shaders/test.frag")});
        pbrShader->setDebugLabel("pbr_shader");
        auto dd_shader = new gl::ShaderPipeline(
            {new gl::ShaderProgram("assets/shaders/direct.vert"),
             new gl::ShaderProgram("assets/shaders/direct.frag")});
        dd_shader->setDebugLabel("direct_buffer/shader");
        directDraw_ = std::make_unique<DirectBuffer>(dd_shader);

        auto fonts = new ui::FontAtlas({{"assets/fonts/MateSC-Medium.ttf",
                                         {{"menu_sm", 20}, {"menu_md", 38}, {"menu_lg", 70}}}},
                                       "menu_md");
        auto skin = ui::loadSkin();
        ui = std::make_unique<ui::Backend>(fonts, skin, new ui::Renderer());
        ui->setViewport(window.size.x, window.size.y);

        imgui = std::make_unique<ui::ImGuiBackend>();
        imgui->setViewport(window.size.x, window.size.y);
        imgui->bind(*input);
    });

    onUnload.push_back([this]() {
        delete skyShader;
        skyShader = nullptr;

        delete pbrShader;
        pbrShader = nullptr;

        imgui->unbind(*input);
    });

    const tinygltf::Model &model = loader::gltf("assets/models/test_course.glb");
    scene = loader::scene(model);
    scene->physics.create(*physics);

    scene::NodeEntityFactory factory;
    factory.registerEntity<CheckpointEntity>("CheckpointEntity");
    factory.registerEntity<TestObstacleEntity>("TestObstacleEntity");
    entityScene = new scene::Scene(*scene, factory);

    entityScene->entities.push_back(new CharacterController(*camera));
}

void Game::run() {
    // Load all the assets
    for (auto &callback : onLoad) {
        callback();
    }

    entityScene->callEntityInit();

    physics->system->OptimizeBroadPhase();

    LOG_INFO("Entering main loop");
    glfwShowWindow(window);
    input->invalidate();
    while (!glfwWindowShouldClose(window)) {
        loop_();
    }
}

void Game::processInput_() {
    bool can_capture_mouse = screen == nullptr && !imgui->shouldShowCursor();
    // Capture mouse
    if (input->isMousePress(GLFW_MOUSE_BUTTON_LEFT) && input->isMouseReleased() && can_capture_mouse) {
        input->captureMouse();
    }
    // Release mouse
    if (input->isKeyPress(GLFW_KEY_ESCAPE) && input->isMouseCaptured()) {
        input->releaseMouse();
    }

    if (!input->isMouseCaptured()) return;

    // Reload assets
    if (input->isKeyPress(GLFW_KEY_F5)) {
        LOG_INFO("Reloading assets");
        for (const auto &callback : onUnload) {
            callback();
        }
        for (const auto &callback : onLoad) {
            callback();
        }
    }

    // Pause / Unpause physics
    if (input->isKeyPress(GLFW_KEY_P)) {
        LOG_INFO("Toggle phyics update");
        physics->setEnabled(!physics->enabled());
    }

    // Open Debug Menu
    if (input->isKeyPress(GLFW_KEY_F3)) {
        LOG_INFO("Toggle Debug Menu");
        debugMenu_->open = !debugMenu_->open;
        debugMenu_->open ? input->releaseMouse() : input->captureMouse();
    }

    // Spawn shpere (Debugging)
    if (input->isKeyPress(GLFW_KEY_L)) {
        LOG_INFO("Spawn shere");
        JPH::BodyCreationSettings sphere_settings(new JPH::SphereShape(0.5f), ph::convert(camera->position - glm::vec3{0.0, 1.0, 0.0}), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, ph::Layers::MOVING);
        sphere_settings.mRestitution = 0.2f;
        physics->interface().CreateAndAddBody(sphere_settings, JPH::EActivation::Activate);
    }
}

void Game::loop_() {
    input->update();
    processInput_();
    ui->update(*input);
    imgui->update(*input);

    tween->update(input->timeDelta());

    debugMenu_->draw();

    nk_context *nk = ui->context();
    // make window transparent
    nk->style.window.background = nk_rgba(0, 0, 0, 0);
    nk->style.window.fixed_background = nk_style_item_color(nk_rgba(0, 0, 0, 0));
    if (nk_begin(nk, "gui", nk_recti(0, 0, window.size.x, window.size.y), 0)) {
        nk_layout_row_dynamic(nk, 30, 2);
        std::chrono::duration<float> total_seconds(input->time());
        auto minutes = std::chrono::duration_cast<std::chrono::minutes>(total_seconds);
        auto seconds = total_seconds - minutes;

        std::string time = std::format("Time: {:02}:{:02}", round(minutes.count()), round(seconds.count()));
        nk_label(nk, time.c_str(), NK_TEXT_ALIGN_LEFT);
    }
    nk_end(nk);

    // Update and step physics
    physics->update(input->timeDelta());
    if (physics->isNextStepDue()) {
        entityScene->callEntityPrePhysicsUpdate();
        physics->step();
        entityScene->callEntityPostPhysicsUpdate();
    }

    // Update entities
    entityScene->callEntityUpdate();

    if (screen != nullptr) {
        screen->draw();
    }

    // Render scene
    gl::manager->setViewport(0, 0, window.size.x, window.size.y);
    gl::manager->disable(gl::Capability::ScissorTest);
    gl::manager->disable(gl::Capability::StencilTest);

    glClearDepth(0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw the loaded instances of the GLTF scene
    // See docs/Rendering.md for a rough explanation
    gl::pushDebugGroup("Game::DrawStatic");
    gl::manager->setEnabled({gl::Capability::DepthTest, gl::Capability::CullFace});
    gl::manager->depthMask(true);
    gl::manager->cullBack();
    gl::manager->depthFunc(gl::DepthFunc::GreaterOrEqual);

    pbrShader->bind();
    pbrShader->vertexStage()->setUniform("u_view_projection_mat", camera->viewProjectionMatrix());
    pbrShader->fragmentStage()->setUniform("u_camera_pos", camera->position);

    scene->graphics.bind();
    for (auto &&batch : scene->graphics.batches) {
        auto &material = scene->graphics.materials[batch.material];
        auto &defaultMaterial = scene->graphics.defaultMaterial;
        pbrShader->fragmentStage()->setUniform("u_albedo_fac", material.albedoFactor);
        pbrShader->fragmentStage()->setUniform("u_metallic_roughness_fac", material.metallicRoughnessFactor);
        if (material.albedo == nullptr) {
            defaultMaterial.albedo->bind(0);
        } else {
            material.albedo->bind(0);
        }
        if (material.occlusionMetallicRoughness == nullptr) {
            defaultMaterial.occlusionMetallicRoughness->bind(1);
        } else {
            material.occlusionMetallicRoughness->bind(1);
        }
        if (material.normal == nullptr) {
            defaultMaterial.normal->bind(2);
        } else {
            material.normal->bind(2);
        }
        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, batch.commandOffset, batch.commandCount, 0);
    }
    gl::popDebugGroup();

    // Draw physics debugging shapes
    physics->debugDraw(camera->viewProjectionMatrix());

    // Draw debug
    directDraw_->draw(camera->viewProjectionMatrix(), camera->position);

    gl::manager->setEnabled({gl::Capability::DepthTest, gl::Capability::DepthClamp});
    gl::manager->depthFunc(gl::DepthFunc::GreaterOrEqual);
    // Draw sky
    quad->bind();
    skyShader->bind();

    skyShader->fragmentStage()->setUniform("u_view_mat", camera->viewMatrix());
    skyShader->fragmentStage()->setUniform("u_projection_mat", camera->projectionMatrix());

    // The sky is rendered using a single, full-screen quad
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Draw UI
    ui->render();
    imgui->render();

    if (screen != nullptr && screen->isClosed()) {
        delete screen;
        screen = nullptr;
    }

    // Finish the frame
    glfwSwapBuffers(window);
}