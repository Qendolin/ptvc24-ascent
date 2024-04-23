#include "Input.h"

#include <algorithm>

#include "Util/Log.h"
#include "Window.h"

Input::Input(Window &window) : window_(window) {
    std::fill(std::begin(mouseButtonsWrite_), std::end(mouseButtonsWrite_), State::Zero);
    std::fill(std::begin(mouseButtonsRead_), std::end(mouseButtonsRead_), State::Zero);
    std::fill(std::begin(keysWrite_), std::end(keysWrite_), State::Zero);
    std::fill(std::begin(keysRead_), std::end(keysRead_), State::Zero);

    for (int key = GLFW_KEY_SPACE; key <= keysWrite_.size(); key++) {
        const char *name = glfwGetKeyName(key, 0);
        if (name != nullptr) {
            keyMap_[name] = key;
        }
    }
}

Input::~Input() = default;

void Input::pollCurrentState_() {
    stateInvalid_ = false;

    for (int key = GLFW_KEY_SPACE; key < keysWrite_.size(); key++) {
        int state = glfwGetKey(window_, key);
        keysWrite_[key] = state == GLFW_PRESS ? State::PersistentPressedMask : State::Zero;
    }

    for (int button = GLFW_MOUSE_BUTTON_1; button < mouseButtonsWrite_.size(); button++) {
        int state = glfwGetMouseButton(window_, button);
        mouseButtonsWrite_[button] = state == GLFW_PRESS ? State::PersistentPressedMask : State::Zero;
    }

    double mouse_x = 0, mouse_y = 0;
    glfwGetCursorPos(window_, &mouse_x, &mouse_y);
    mousePosWrite_ = {mouse_x, mouse_y};
    // No mouse delta
    mousePosRead_ = mousePosWrite_;

    mouseCaptured_ = glfwGetInputMode(window_, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;

    // No time delta
    timeRead_ = glfwGetTime();
}

void Input::update() {
    glfwPollEvents();

    if (stateInvalid_) {
        pollCurrentState_();
    }

    double time = glfwGetTime();
    timeDelta_ = static_cast<float>(time - timeRead_);
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
        keysWrite_[i] &= State::ClearMask;
    }

    // Same for mouse buttons
    std::copy(std::begin(mouseButtonsWrite_), std::end(mouseButtonsWrite_), std::begin(mouseButtonsRead_));
    for (size_t i = 0; i < mouseButtonsWrite_.size(); i++) {
        mouseButtonsWrite_[i] &= State::ClearMask;
    }
}

void Input::captureMouse() {
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    mouseCaptured_ = true;
}

void Input::releaseMouse() {
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    mouseCaptured_ = false;
}

void Input::centerMouse() {
    int w, h;
    glfwGetWindowSize(window_, &w, &h);
    glfwSetCursorPos(window_, w / 2, h / 2);
}

bool Input::isWindowFocused() {
    return glfwGetWindowAttrib(window_, GLFW_FOCUSED) == GLFW_TRUE;
}

void Input::onKey(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key < 0 || key >= keysWrite_.size()) return;  // special keys, e.g.: mute sound
    if (action == GLFW_PRESS) {
        // set the pressed and down bit
        keysWrite_[key] |= State::PressedBit;
        keysWrite_[key] |= State::PersistentPressedBit;
    }

    if (action == GLFW_RELEASE) {
        // set the released and clear the down bit
        keysWrite_[key] |= State::ReleasedBit;
        keysWrite_[key] &= static_cast<State>(~static_cast<uint8_t>(State::PersistentPressedBit));
    }

    for (auto &&reg : keyCallbacks_)
        reg.callback(key, scancode, action, mods);
}

void Input::onCursorPos(GLFWwindow *window, double x, double y) {
    mousePosWrite_.x = static_cast<float>(x);
    mousePosWrite_.y = static_cast<float>(y);

    for (auto &&reg : mousePosCallbacks_)
        reg.callback(static_cast<float>(x), static_cast<float>(y));
}

void Input::onMouseButton(GLFWwindow *window, int button, int action, int mods) {
    if (action == GLFW_PRESS) {
        // set the pressed and down bit
        mouseButtonsWrite_[button] |= State::PressedBit;
        mouseButtonsWrite_[button] |= State::PersistentPressedBit;
    }

    if (action == GLFW_RELEASE) {
        // set the released and clear the down bit
        mouseButtonsWrite_[button] |= State::ReleasedBit;
        mouseButtonsWrite_[button] &= static_cast<State>(~static_cast<uint8_t>(State::PersistentPressedBit));
    }

    for (auto &&reg : mouseButtonCallbacks_)
        reg.callback(button, action, mods);
}

void Input::onScroll(GLFWwindow *window, double dx, double dy) {
    scrollDeltaWrite_.x += static_cast<float>(dx);
    scrollDeltaWrite_.y += static_cast<float>(dy);

    for (auto &&reg : scrollCallbacks_)
        reg.callback(static_cast<float>(dx), static_cast<float>(dy));
}

void Input::onChar(GLFWwindow *window, unsigned int codepoint) {
    for (auto &&reg : charCallbacks_)
        reg.callback(codepoint);
}

// a little helper function
template <typename T>
void removeCallbackFromVec(std::vector<T> &vec, Input::CallbackRegistrationID id) {
    vec.erase(
        std::remove_if(
            vec.begin(), vec.end(),
            [&id](T &elem) { return elem.id == id; }),
        vec.end());
}

void Input::removeCallback(CallbackRegistrationID &registration) {
    if (registration == 0) {
        LOG_WARN("removeCallback called with invalid registration id (0)");
    }
    removeCallbackFromVec(mousePosCallbacks_, registration);
    removeCallbackFromVec(mouseButtonCallbacks_, registration);
    removeCallbackFromVec(scrollCallbacks_, registration);
    removeCallbackFromVec(keyCallbacks_, registration);
    removeCallbackFromVec(charCallbacks_, registration);
    registration = 0;
}