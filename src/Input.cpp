#include "Input.h"

#include <algorithm>

// Why must this be so cumbersome?
constexpr inline Input::State operator|(Input::State lhs, Input::State rhs) {
    return static_cast<Input::State>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}
constexpr inline Input::State operator&(Input::State lhs, Input::State rhs) {
    return static_cast<Input::State>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}
constexpr inline Input::State &operator|=(Input::State &lhs, Input::State rhs) {
    return lhs = lhs | rhs;
}
constexpr inline Input::State &operator&=(Input::State &lhs, Input::State rhs) {
    return lhs = lhs & rhs;
}

Input::Input() {
    std::fill(std::begin(mouseButtonsWrite_), std::end(mouseButtonsWrite_), State::ZERO);
    std::fill(std::begin(mouseButtonsRead_), std::end(mouseButtonsRead_), State::ZERO);
    std::fill(std::begin(keysWrite_), std::end(keysWrite_), State::ZERO);
    std::fill(std::begin(keysRead_), std::end(keysRead_), State::ZERO);
}

void Input::update() {
    glfwPollEvents();

    time_ = glfwGetTime();
    timeDelta_ = time_ - prevTime_;
    prevTime_ = time_;

    mouseDelta_ = mousePosWrite_ - mousePosRead_;
    mousePosRead_ = mousePosWrite_;

    scrollDeltaRead_ = scrollDeltaWrite_;
    scrollDeltaWrite_ = glm::vec2(0.0f);

    // During a frame key events are captured and flags set in the keysWrite_ buffer.
    // After a frame the keysWrite_ buffer is copied to the keysRead_ buffer and then cleared.
    std::copy(std::begin(keysWrite_), std::end(keysWrite_), std::begin(keysRead_));
    // The state changes (pressed, released) are cleared but the current state is kept
    for (size_t i = 0; i < keysWrite_.size(); i++) {
        keysWrite_[i] &= State::CLEAR_MASK;
    }

    // Same for mouse buttons
    std::copy(std::begin(mouseButtonsWrite_), std::end(mouseButtonsWrite_), std::begin(mouseButtonsRead_));
    for (size_t i = 0; i < mouseButtonsWrite_.size(); i++) {
        mouseButtonsWrite_[i] &= State::CLEAR_MASK;
    }
}

void Input::onKey(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        // set the pressed and down bit
        keysWrite_[key] |= State::PRESSED_BIT;
        keysWrite_[key] |= State::PERSISTENT_PRESSED_BIT;
    }

    if (action == GLFW_RELEASE) {
        // set the released and clear the down bit
        keysWrite_[key] |= State::RELEASED_BIT;
        keysWrite_[key] &= static_cast<State>(~static_cast<uint8_t>(State::PERSISTENT_PRESSED_BIT));
    }
}

void Input::onCursorPos(GLFWwindow *window, double x, double y) {
    mousePosWrite_.x = x;
    mousePosWrite_.y = y;
}

void Input::onMouseButton(GLFWwindow *window, int button, int action, int mods) {
    if (action == GLFW_PRESS) {
        // set the pressed and down bit
        mouseButtonsWrite_[button] |= State::PRESSED_BIT;
        mouseButtonsWrite_[button] |= State::PERSISTENT_PRESSED_BIT;
    }

    if (action == GLFW_RELEASE) {
        // set the released and clear the down bit
        mouseButtonsWrite_[button] |= State::RELEASED_BIT;
        mouseButtonsWrite_[button] &= static_cast<State>(~static_cast<uint8_t>(State::PERSISTENT_PRESSED_BIT));
    }
}

void Input::onScroll(GLFWwindow *window, double dx, double dy) {
    scrollDeltaWrite_.x += dx;
    scrollDeltaWrite_.y += dy;
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