#include "Input.h"

#include <algorithm>

Input::Input(GLFWwindow *window) : window_(window) {
    std::fill(std::begin(mouseButtonsWrite_), std::end(mouseButtonsWrite_), State::ZERO);
    std::fill(std::begin(mouseButtonsRead_), std::end(mouseButtonsRead_), State::ZERO);
    std::fill(std::begin(keysWrite_), std::end(keysWrite_), State::ZERO);
    std::fill(std::begin(keysRead_), std::end(keysRead_), State::ZERO);

    for (int key = GLFW_KEY_SPACE; key <= keysWrite_.size(); key++) {
        const char *name = glfwGetKeyName(key, 0);
        if (name != nullptr) {
            keyMap_[name] = key;
        }
    }
}

void Input::pollCurrentState_() {
    stateInvalid_ = false;

    for (int key = GLFW_KEY_SPACE; key < keysWrite_.size(); key++) {
        int state = glfwGetKey(window_, key);
        keysWrite_[key] = state == GLFW_PRESS ? State::PERSISTENT_PRESSED_MASK : State::ZERO;
    }

    for (int button = GLFW_MOUSE_BUTTON_1; button < mouseButtonsWrite_.size(); button++) {
        int state = glfwGetMouseButton(window_, button);
        mouseButtonsWrite_[button] = state == GLFW_PRESS ? State::PERSISTENT_PRESSED_MASK : State::ZERO;
    }

    double mouse_x = 0, mouse_y = 0;
    glfwGetCursorPos(window_, &mouse_x, &mouse_y);
    mousePosWrite_ = {mouse_x, mouse_y};
    // No mouse delta
    mousePosRead_ = mousePosWrite_;

    mouseCaptured_ = glfwGetInputMode(window_, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;
}

void Input::update() {
    glfwPollEvents();

    if (stateInvalid_) {
        pollCurrentState_();
    }

    float time = glfwGetTime();
    timeDelta_ = time - timeRead_;
    timeRead_ = time;

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