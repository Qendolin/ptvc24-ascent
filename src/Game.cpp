#include "Game.h"

#include <chrono>

#include "GL/StateManager.h"
#include "Physics/Shapes.h"
#include "Scene/Character.h"
#include "Scene/Objects/Checkpoint.h"
#include "Scene/Objects/TestObstacle.h"
#include "UI/Screens/DebugMenu.h"
#include "UI/Screens/MainMenu.h"
#include "UI/Skin.h"
#include "Utils.h"

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

Game::Game(GLFWwindow *window) {
    this->window = window;

    input = Input::instance = new Input(window);
    glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode, int action, int mods) {
        Input::instance->onKey(window, key, scancode, action, mods);
    });
    glfwSetCursorPosCallback(window, [](GLFWwindow *window, double x, double y) {
        Input::instance->onCursorPos(window, x, y);
    });
    glfwSetMouseButtonCallback(window, [](GLFWwindow *window, int button, int action, int mods) {
        Input::instance->onMouseButton(window, button, action, mods);
    });
    glfwSetScrollCallback(window, [](GLFWwindow *window, double dx, double dy) {
        Input::instance->onScroll(window, dx, dy);
    });
    glfwSetWindowFocusCallback(window, [](GLFWwindow *window, int focused) {
        Input::instance->invalidate();
    });

    // Create camera with a 90° vertical FOV
    camera = new Camera(glm::radians(90.0f), viewportSize, 0.1f, glm::vec3{0, 1, 1}, glm::vec3{});

    // window / viewport size
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int width, int height) {
        Game::instance->resize(width, height);
    });
    int vp_width, vp_height;
    glfwGetFramebufferSize(window, &vp_width, &vp_height);
    Game::resize(vp_width, vp_height);

    physics = new ph::Physics({});
    physics->setDebugDrawEnabled(true);
}

Game::~Game() {
    window = nullptr;
    quad->destroy();
    quad = nullptr;

    dd->destroy();
    dd = nullptr;

    skyShader->destroy();
    skyShader = nullptr;
    pbrShader->destroy();
    pbrShader = nullptr;

    // delete entities before deleting the physics
    if (entityScene != nullptr) delete entityScene;
    entityScene = nullptr;

    if (scene != nullptr) delete scene;
    scene = nullptr;

    if (screen != nullptr) delete screen;
    screen = nullptr;

    delete physics;
    physics = nullptr;
    delete ui;
    ui = nullptr;
    delete input;
    input = nullptr;
    delete camera;
    camera = nullptr;
}

void Game::resize(int width, int height) {
    viewportSize = glm::ivec2(width, height);

    if (camera != nullptr)
        camera->setViewportSize(viewportSize);

    float x_scale, y_scale;
    glfwGetWindowContentScale(window, &x_scale, &y_scale);
    ui::set_scale(width, height, x_scale);

    if (ui != nullptr)
        ui->setViewport(viewportSize);
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
        dd = new DirectBuffer(dd_shader);

        auto fonts = new ui::FontAtlas({{"assets/fonts/MateSC-Medium.ttf",
                                         {{"menu_sm", 20}, {"menu_md", 38}, {"menu_lg", 70}}}},
                                       "menu_md");
        auto skin = ui::loadSkin();
        ui = new ui::Backend(fonts, skin, new ui::Renderer());
        ui->setViewport(viewportSize);
    });

    onUnload.push_back([this]() {
        if (skyShader) {
            skyShader->destroy();
            skyShader = nullptr;
        }
        if (pbrShader) {
            pbrShader->destroy();
            pbrShader = nullptr;
        }
        if (dd) {
            dd->destroy();
            dd = nullptr;
        }
        if (ui) {
            delete ui;
            ui = nullptr;
        }
    });

    const tinygltf::Model &model = loader::gltf("assets/models/test_course.glb");
    scene = loader::scene(model);
    scene->physics.create(*physics);

    scene::NodeEntityFactory factory;
    factory.registerEntity<CheckpointEntity>("CheckpointEntity");
    factory.registerEntity<TestObstacleEntity>("TestObstacleEntity");
    entityScene = new scene::Scene(*scene, factory);

    entityScene->entities.push_back(new CharacterController(camera));
}

void Game::run() {
    // Load all the assets
    for (auto &callback : onLoad) {
        callback();
    }

    entityScene->callEntityInit();

    physics->system->OptimizeBroadPhase();

    LOG("Entering main loop");
    glfwShowWindow(window);
    input->invalidate();
    while (!glfwWindowShouldClose(window)) {
        loop_();
    }
}

void Game::processInput_() {
    // Capture mouse
    if (input->isMousePress(GLFW_MOUSE_BUTTON_LEFT) && input->isMouseReleased() && screen == nullptr) {
        input->captureMouse();
    }
    // Release mouse
    if (input->isKeyPress(GLFW_KEY_ESCAPE) && input->isMouseCaptured()) {
        input->releaseMouse();
    }

    if (!input->isMouseCaptured()) return;

    // Reload assets
    if (input->isKeyPress(GLFW_KEY_F5)) {
        LOG("Reloading assets");
        for (const auto &callback : onUnload) {
            callback();
        }
        for (const auto &callback : onLoad) {
            callback();
        }
    }

    // Pause / Unpause physics
    if (input->isKeyPress(GLFW_KEY_P)) {
        LOG("Toggle phyics update");
        physics->setEnabled(!physics->enabled());
    }

    // Open Debug Menu
    if (input->isKeyPress(GLFW_KEY_F3)) {
        LOG("Open Debug Menu");
        screen = new DebugMenuScreen();
    }

    // Spawn shpere (Debugging)
    if (input->isKeyPress(GLFW_KEY_L)) {
        LOG("Spawn shere");
        JPH::BodyCreationSettings sphere_settings(new JPH::SphereShape(0.5f), ph::convert(camera->position - glm::vec3{0.0, 1.0, 0.0}), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, ph::Layers::MOVING);
        sphere_settings.mRestitution = 0.2;
        physics->interface().CreateAndAddBody(sphere_settings, JPH::EActivation::Activate);
    }
}

void Game::loop_() {
    input->update();
    processInput_();
    ui->update(input);

    tween.update(input->timeDelta());

    nk_context *nk = ui->context();
    // make window transparent
    nk->style.window.background = nk_rgba(0, 0, 0, 0);
    nk->style.window.fixed_background = nk_style_item_color(nk_rgba(0, 0, 0, 0));
    if (nk_begin(nk, "gui", nk_rect(0, 0, viewportSize.x, viewportSize.y), 0)) {
        nk_layout_row_dynamic(nk, 30, 2);
        std::chrono::duration<float> total_seconds(input->time());
        auto minutes = std::chrono::duration_cast<std::chrono::minutes>(total_seconds);
        auto seconds = total_seconds - minutes;

        std::string time = std::format("Time: {:02}:{:02}", round(minutes.count()), round(seconds.count()));
        nk_label(nk, time.c_str(), NK_TEXT_ALIGN_LEFT);

        float fps = floor(1.0 / input->timeDelta());
        nk_label(nk, std::format("{:4.0f}fps {:5.2f}ms", fps, input->timeDelta() * 1000.0).c_str(), NK_TEXT_ALIGN_RIGHT);
    }
    nk_end(nk);
    // drawMenu(this);

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
    gl::manager->setViewport(0, 0, viewportSize.x, viewportSize.y);
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
    dd->draw(camera->viewProjectionMatrix(), camera->position);

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

    if (screen != nullptr && screen->isClosed()) {
        delete screen;
        screen = nullptr;
    }

    // Finish the frame
    glfwSwapBuffers(window);
}