#include "Game.h"

#include <imgui.h>

#include <glm/glm.hpp>

#include "Camera.h"
#include "Controller/MainMenuController.h"
#include "Debug/DebugMenu.h"
#include "Debug/Direct.h"
#include "Debug/ImGuiBackend.h"
#include "GL/Framebuffer.h"
#include "GL/StateManager.h"
#include "GL/Texture.h"
#include "Input.h"
#include "Physics/Physics.h"
#include "Renderer/FinalizationRenderer.h"
#include "Tween.h"
#include "UI/Renderer.h"
#include "UI/Skin.h"
#include "UI/UI.h"
#include "Utils.h"
#include "Window.h"

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
    camera = std::make_unique<Camera>(glm::radians(90.0f), 0.1f, glm::vec3{0, 1, 1}, glm::vec3{});

    // window / viewport size
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int width, int height) {
        Game::get().resize(width, height);
    });

    ImGui::SetCurrentContext(ImGui::CreateContext());

    physics = std::make_unique<ph::Physics>();
    physics->setDebugDrawEnabled(true);

    queueController<MainMenuController>();

    hdrFramebuffer_ = new gl::Framebuffer();
    hdrFramebuffer_->setDebugLabel("hdr_fbo");

    // at the end
    int vp_width, vp_height;
    glfwGetFramebufferSize(window, &vp_width, &vp_height);
    Game::resize(vp_width, vp_height);
}

Game::~Game() {
    controller->unload();

    delete hdrFramebuffer_;

    ImGui::DestroyContext();
}

void Game::resize(int width, int height) {
    window.size.x = width;
    window.size.y = height;

    camera->setViewport(width, height);

    float x_scale, y_scale;
    glfwGetWindowContentScale(window, &x_scale, &y_scale);
    ui::set_scale(width, height, x_scale);

    if (ui != nullptr)
        ui->setViewport(width, height);

    if (imgui != nullptr)
        imgui->setViewport(width, height);

    delete hdrFramebuffer_->getTexture(0);
    auto hdr_color_attachment = new gl::Texture(GL_TEXTURE_2D);
    hdr_color_attachment->setDebugLabel("hdr_fbo/color");
    hdr_color_attachment->allocate(1, GL_R11F_G11F_B10F, width, height);
    hdrFramebuffer_->attachTexture(0, hdr_color_attachment);
    hdrFramebuffer_->bindTargets({0});

    delete hdrFramebuffer_->getTexture(GL_DEPTH_ATTACHMENT);
    auto hdr_depth_attachment = new gl::Texture(GL_TEXTURE_2D);
    hdr_depth_attachment->setDebugLabel("hdr_fbo/depth");
    hdr_depth_attachment->allocate(1, GL_DEPTH_COMPONENT24, width, height);
    hdrFramebuffer_->attachTexture(GL_DEPTH_ATTACHMENT, hdr_depth_attachment);
    hdrFramebuffer_->check(GL_DRAW_FRAMEBUFFER);
}

void Game::load() {
    directDraw = std::make_unique<DirectBuffer>();

    auto fonts = new ui::FontAtlas({{"assets/fonts/MateSC-Medium.ttf",
                                     {{"menu_sm", 20}, {"menu_md", 38}, {"menu_lg", 70}}}},
                                   "menu_md");
    auto skin = ui::loadSkin();
    ui = std::make_unique<ui::Backend>(fonts, skin, new ui::Renderer());
    ui->setViewport(window.size.x, window.size.y);

    imgui = std::make_unique<ui::ImGuiBackend>();
    imgui->setViewport(window.size.x, window.size.y);
    imgui->bind(*input);

    if (controller != nullptr)
        controller->load();

    finalizationRenderer_ = std::make_unique<FinalizationRenderer>();
}

void Game::unload() {
    if (controller != nullptr)
        controller->unload();

    imgui->unbind(*input);
}

void Game::run() {
    LOG_INFO("Entering main loop");
    glfwShowWindow(window);
    input->invalidate();
    while (!glfwWindowShouldClose(window)) {
        update_();
        render_();
    }
}

void Game::processInput_() {
    if (!input->isWindowFocused()) return;

    // Reload assets
    if (input->isKeyPress(GLFW_KEY_F5)) {
        LOG_INFO("Reloading assets");
        unload();
        load();
    }

    // Open Debug Menu
    if (input->isKeyPress(GLFW_KEY_F3)) {
        LOG_INFO("Toggle Debug Menu");
        debugMenu_->open = !debugMenu_->open;
        debugMenu_->open ? input->releaseMouse() : input->captureMouse();
    }
}

void Game::update_() {
    input->update();
    processInput_();
    ui->update(*input);
    imgui->update(*input);
    tween->update(input->timeDelta());

    if (queuedController_ != nullptr) {
        if (controller != nullptr)
            controller->unload();
        controller = std::move(queuedController_);
        controller->load();
    }

    controller->update();
    debugMenu_->draw();
}

void Game::render_() {
    // Clear buffer
    gl::manager->setViewport(0, 0, window.size.x, window.size.y);
    gl::manager->disable(gl::Capability::ScissorTest);
    gl::manager->disable(gl::Capability::StencilTest);

    hdrFramebuffer_->bind(GL_DRAW_FRAMEBUFFER);

    glClearDepth(0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    controller->render();

    gl::manager->bindDrawFramebuffer(0);
    finalizationRenderer_->render(hdrFramebuffer_->getTexture(0), hdrFramebuffer_->getTexture(GL_DEPTH_ATTACHMENT));

    // Draw physics debugging shapes
    physics->debugRender(camera->viewProjectionMatrix());

    // Draw debug
    directDraw->render(camera->viewProjectionMatrix(), camera->position);

    // Draw UI
    ui->render();
    imgui->render();

    // Finish the frame
    glfwSwapBuffers(window);
}