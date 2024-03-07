#include "Input.h"

void Input::update() {
    time_ = glfwGetTime();
    timeDelta_ = time_ - prevTime_;
    prevTime_ = time_;
    mouseDelta_ = mousePos_ - mousePrevPos_;
    mousePrevPos_ = mousePos_;
    scrollDelta_ = scrollNextDelta_;
    scrollNextDelta_ = glm::vec2(0.0f);
    std::copy(std::begin(mouseButtons_), std::end(mouseButtons_), std::begin(mousePrevButtons_));
    std::copy(std::begin(keys_), std::end(keys_), std::begin(keysPrev_));
}

void Input::onKey(GLFWwindow *window, int key, int scancode, int action, int mods) {
    keys_[key] = action != GLFW_RELEASE;
}

void Input::onCursorPos(GLFWwindow *window, double x, double y) {
    mousePos_.x = x;
    mousePos_.y = y;
}

void Input::onMouseButton(GLFWwindow *window, int button, int action, int mods) {
    mouseButtons_[button] = action != GLFW_RELEASE;
}

void Input::onScroll(GLFWwindow *window, double dx, double dy) {
    scrollNextDelta_.x += dx;
    scrollNextDelta_.y += dy;
}

Input *Input::instance() {
    return Input::instance_;
}

Input *Input::init(GLFWwindow *window) {
    Input::instance_ = new Input();
    glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode, int action, int mods) { Input::instance_->onKey(window, key, scancode, action, mods); });
    glfwSetCursorPosCallback(window, [](GLFWwindow *window, double x, double y) { Input::instance_->onCursorPos(window, x, y); });
    glfwSetMouseButtonCallback(window, [](GLFWwindow *window, int button, int action, int mods) { Input::instance_->onMouseButton(window, button, action, mods); });
    glfwSetScrollCallback(window, [](GLFWwindow *window, double dx, double dy) { Input::instance_->onScroll(window, dx, dy); });
    return Input::instance_;
}

Input *Input::instance_ = nullptr;