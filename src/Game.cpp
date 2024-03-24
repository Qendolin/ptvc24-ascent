#include "Game.h"

#include <chrono>

#include "GL/StateManager.h"
// #include "Menu.h"
#include "UI/Screens/MainMenu.h"
#include "UI/Style.h"
#include "Utils.h"

GL::VertexArray *createQuad() {
    GL::Buffer *vbo = new GL::Buffer();
    glm::vec2 quad_verts[] = {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}};
    vbo->allocate(&quad_verts, sizeof(quad_verts), 0);

    GL::VertexArray *quad = new GL::VertexArray();
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
    camera = new Camera(glm::radians(90.), viewportSize, 0.1, glm::vec3{0, 1, 1}, glm::vec3{});

    // window / viewport size
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int width, int height) {
        Game::instance->resize(width, height);
    });
    int vp_width, vp_height;
    glfwGetFramebufferSize(window, &vp_width, &vp_height);
    Game::resize(vp_width, vp_height);

    physics = new PH::Physics({});
}

void Game::resize(int width, int height) {
    viewportSize = glm::ivec2(width, height);
    camera->setViewportSize(viewportSize);
    float x_scale, y_scale;
    glfwGetWindowContentScale(window, &x_scale, &y_scale);
    NK::set_scale(width, height, x_scale);
}

// This is temporary
void createPhysicsScene(PH::Physics *physics) {
    JPH::BodyInterface &interface = physics->interface();
    JPH::BodyCreationSettings sphere_settings(new JPH::SphereShape(0.5f), JPH::RVec3(0.0f, 5.0f, -4.0f), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, PH::Layers::MOVING);
    // make it bouncy
    sphere_settings.mRestitution = 0.9;
    interface.CreateAndAddBody(sphere_settings, JPH::EActivation::Activate);

    JPH::BoxShapeSettings floor_shape_settings(JPH::Vec3(100.0f, 1.0f, 100.0f));
    JPH::ShapeRefC floor_shape = floor_shape_settings.Create().Get();
    JPH::BodyCreationSettings floor_settings(floor_shape, JPH::RVec3(0.0, -1.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Static, PH::Layers::NON_MOVING);
    interface.CreateAndAddBody(floor_settings, JPH::EActivation::DontActivate);

    JPH::BoxShapeSettings sensor_test_shape_settings(JPH::Vec3(2.0f, 2.0f, 2.0f));
    JPH::ShapeRefC sensor_test_shape = sensor_test_shape_settings.Create().Get();
    JPH::BodyCreationSettings sensor_test_settings(sensor_test_shape, JPH::RVec3(5.0, 0.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Static, PH::Layers::SENSOR);
    sensor_test_settings.mIsSensor = true;
    JPH::BodyID sensor_test_id = interface.CreateAndAddBody(sensor_test_settings, JPH::EActivation::DontActivate);
    physics->contactListener->RegisterCallback(sensor_test_id, [physics](PH::SensorContact contact) {
        if (contact.persistent) return;
        LOG("Sensor activated");
        // JPH::BodyInterface &interface = physics->interface();
        // interface.RemoveBody(contact.sensor);
        // interface.DestroyBody(contact.sensor);
        // physics->contactListener->UnrgisterCallback(contact.sensor);
    });
}

void Game::setup() {
    quad = createQuad();

    screen = new MainMenuScreen();

    // defer loading assets
    onLoad.push_back([this]() {
        skyShader = new GL::ShaderPipeline(
            {new GL::ShaderProgram("assets/shaders/sky.vert"),
             new GL::ShaderProgram("assets/shaders/sky.frag")});
        pbrShader = new GL::ShaderPipeline(
            {new GL::ShaderProgram("assets/shaders/test.vert"),
             new GL::ShaderProgram("assets/shaders/test.frag")});
        auto dd_shader = new GL::ShaderPipeline(
            {new GL::ShaderProgram("assets/shaders/direct.vert"),
             new GL::ShaderProgram("assets/shaders/direct.frag")});
        dd = new DirectBuffer(dd_shader);

        fonts = new NK::FontAtlas({{"assets/fonts/MateSC-Medium.ttf",
                                    {{"menu_sm", 20}, {"menu_md", 38}, {"menu_lg", 70}}}},
                                  "menu_md");
        ui = new NK::Backend(fonts);

        // TODO: this should be part of ui, also cleanup
        Skin skin = load_skin();
        skin.apply(ui->context());
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
        if (fonts) {
            delete fonts;
            fonts = nullptr;
        }
    });

    scene = Loader::gltf("assets/models/pbr_test.glb");
    createPhysicsScene(physics);

    entities.push_back(new CharacterController(camera));
}

void Game::run() {
    // Load all the assets
    for (const auto &callback : onLoad) {
        callback();
    }

    for (auto &&ent : entities) {
        ent->init();
    }

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
}

void Game::loop_() {
    input->update();
    processInput_();
    ui->update(input);

    tweenTimer_ += input->timeDelta() * 1000.0;
    // LOG(tweenTimer_);
    tweenTimeStep_ = (int)floor(tweenTimer_);
    tweenTimer_ -= tweenTimeStep_;

    nk_context *nk = ui->context();
    struct nk_style *s = &nk->style;
    // make window transparent
    // window_style_transparent(nk);
    if (nk_begin(nk, "gui", nk_rect(0, 0, viewportSize.x, viewportSize.y), 0)) {
        nk_layout_row_dynamic(nk, 30, 1);
        std::chrono::duration<float> total_seconds(input->time());
        auto minutes = std::chrono::duration_cast<std::chrono::minutes>(total_seconds);
        auto seconds = total_seconds - minutes;

        std::string time = std::format("Time: {:02}:{:02}", round(minutes.count()), round(seconds.count()));
        nk_label(nk, time.c_str(), NK_TEXT_ALIGN_LEFT);
    }
    nk_end(nk);
    // drawMenu(this);

    // Update and step physics
    physics->update(input->timeDelta());
    if (physics->isNextStepDue()) {
        for (auto &&ent : entities) {
            ent->prePhysicsUpdate();
        }

        physics->step();

        for (auto &&ent : entities) {
            ent->postPhysicsUpdate();
        }
    }

    // Update entities
    for (auto &&ent : entities) {
        ent->update();
    }

    if (screen != nullptr) {
        screen->draw();
    }

    // Render scene
    GL::manager->setViewport(0, 0, viewportSize.x, viewportSize.y);
    GL::manager->disable(GL::Capability::ScissorTest);
    GL::manager->disable(GL::Capability::StencilTest);

    glClearDepth(0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw the loaded instances of the GLTF scene
    GL::manager->setEnabled({GL::Capability::DepthTest});
    GL::manager->depthMask(true);
    GL::manager->depthFunc(GL::DepthFunc::GreaterOrEqual);

    pbrShader->bind();
    pbrShader->vertexStage()->setUniform("u_view_projection_mat", camera->viewProjectionMatrix());
    pbrShader->fragmentStage()->setUniform("u_camera_pos", camera->position);

    for (auto &node : scene) {
        node.mesh.vao->bind();
        pbrShader->vertexStage()->setUniform("u_model_mat", node.transform);
        for (auto &s : node.mesh.sections) {
            pbrShader->fragmentStage()->setUniform("u_albedo_fac", s.material.albedoFactor);
            pbrShader->fragmentStage()->setUniform("u_metallic_roughness_fac", s.material.metallicRoughnessFactor);
            if (s.material.albedo != nullptr)
                s.material.albedo->bind(0);
            if (s.material.occlusionMetallicRoughness != nullptr)
                s.material.occlusionMetallicRoughness->bind(1);
            if (s.material.normal != nullptr)
                s.material.normal->bind(2);
            glDrawElementsBaseVertex(GL_TRIANGLES, s.length, GL_UNSIGNED_SHORT, 0, s.base);
        }
    }

    // Draw physics debugging shapes
    physics->debugDraw(camera->viewProjectionMatrix());

    // Draw debug
    dd->draw(camera->viewProjectionMatrix(), camera->position);

    GL::manager->setEnabled({GL::Capability::DepthTest, GL::Capability::DepthClamp});
    GL::manager->depthFunc(GL::DepthFunc::GreaterOrEqual);
    // Draw sky
    quad->bind();
    skyShader->bind();

    skyShader->fragmentStage()->setUniform("u_view_mat", camera->viewMatrix());
    skyShader->fragmentStage()->setUniform("u_projection_mat", camera->projectionMatrix());

    // The sky is rendered using a single, full-screen quad
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Draw UI
    glm::mat4 projection_matrix = glm::mat4(
        2.0f / viewportSize.x, 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f / viewportSize.y, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f);

    ui->render(projection_matrix, viewportSize);

    // Finish the frame
    glfwSwapBuffers(window);
}