#pragma once

#include <GLFW/glfw3.h>

#include <array>
#include <glm/glm.hpp>
#include <iterator>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * The input class handles user input.
 * There quite a few considerations to be made:
 * - An input should register, no matter how short it is
 *   This is solved by using GLFW's input callbacks and using "latching" bits.
 *   There is a latching bit for presses and releases. They are only reset during the update,
 *   ensuring that a key tap during a lag frame still registers as a press and release.
 *
 * - An input should register, no matter how late or early it is
 *   This is solved by using double buffering.
 *
 * - Querying the input state must be consistent / idempotent during a frame.
 *   Meaning: it doesn't matter when the state is queried, during a frame it will always return the same result.
 *   This is solved by using double buffering.
 *
 * - Multiple input actions (press / release) during a frame must all be registered.
 *   This is partially solved by using "latching" bits as explained above.
 *   This allows for one press and release action during a frame.
 *   This is a fine compromise since more than one tap per frame is unlikely.
 *
 * Note:
 *  - isKeyPressed() == true does not imply that isKeyReleased() == false, both can be true.
 */
class Input {
   private:
    enum class State : uint8_t {
        ZERO = 0,
        RELEASED_BIT = 0b001,
        PRESSED_BIT = 0b010,
        PERSISTENT_PRESSED_BIT = 0b100,
        CLEAR_MASK = static_cast<State>(~static_cast<uint8_t>(RELEASED_BIT | PRESSED_BIT)),
        PERSISTENT_PRESSED_MASK = PRESSED_BIT | PERSISTENT_PRESSED_BIT,
    };

    // Operators need to defined explicitly for enums. This allows bitwise operations like `&` and `|`.
    // Why must this be so cumbersome?
    friend constexpr inline State operator|(State lhs, State rhs) {
        return static_cast<Input::State>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
    }
    friend constexpr inline State operator&(State lhs, State rhs) {
        return static_cast<Input::State>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
    }
    friend constexpr inline State &operator|=(State &lhs, State rhs) {
        return lhs = lhs | rhs;
    }
    friend constexpr inline State &operator&=(State &lhs, State rhs) {
        return lhs = lhs & rhs;
    }

    GLFWwindow *window_ = nullptr;
    float timeRead_ = 0;
    float timeDelta_ = 0;
    bool mouseCaptured_ = false;
    glm::vec2 mousePosRead_ = {};
    glm::vec2 mousePosWrite_ = {};
    glm::vec2 mouseDelta_ = {};
    glm::vec2 scrollDeltaRead_ = {};
    glm::vec2 scrollDeltaWrite_ = {};
    std::array<State, GLFW_MOUSE_BUTTON_LAST + 1> mouseButtonsRead_;
    std::array<State, GLFW_MOUSE_BUTTON_LAST + 1> mouseButtonsWrite_;
    std::array<State, GLFW_KEY_LAST + 1> keysRead_;
    std::array<State, GLFW_KEY_LAST + 1> keysWrite_;
    std::unordered_map<std::string, int> keyMap_ = {};

    bool stateInvalid_ = true;

    /**
     * Polls every key and mouse button to ensure that the internal state is up to date.
     * Is called after invalidate.
     */
    void pollCurrentState_();

   public:
    inline static Input *instance = nullptr;

    Input(GLFWwindow *window);
    ~Input();

    /**
     * @return the mouse position
     */
    const glm::vec2 mousePos() { return mousePosRead_; }
    /**
     * @return the mouse position difference since the last frame
     */
    const glm::vec2 mouseDelta() { return mouseDelta_; }
    /**
     * @return the scroll wheel position difference since the last frame
     */
    const glm::vec2 scrollDelta() { return scrollDeltaRead_; }
    /**
     * @return the time difference since the last frame
     */
    const float timeDelta() { return timeDelta_; }
    /**
     * @return the time since GLFW was initialized
     */
    const float time() { return timeRead_; }

    /**
     * @return `true` if the mouse is captured (aka. grabbed).
     */
    bool isMouseCaptured() {
        return mouseCaptured_;
    }

    void captureMouse() {
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        mouseCaptured_ = true;
    }

    /**
     * Opposite of isMouseCaptured
     */
    bool isMouseReleased() {
        return !mouseCaptured_;
    }

    void releaseMouse() {
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        mouseCaptured_ = false;
    }

    /**
     * @param button one of `GLFW_MOUSE_BUTTON_*`
     * @return `true` if the given button is being held down.
     */
    bool isMouseDown(int button) const {
        return (mouseButtonsRead_[button] & State::PERSISTENT_PRESSED_MASK) != State::ZERO;
    }

    /**
     * @param button one of `GLFW_MOUSE_BUTTON_*`
     * @return `true` if the given button has been pressed down since the last frame.
     */
    bool isMousePress(int button) const {
        return (mouseButtonsRead_[button] & State::PRESSED_BIT) != State::ZERO;
    }

    /**
     * @param button one of `GLFW_MOUSE_BUTTON_*`
     * @return `true` if the given button has been pressed down since the last frame.
     */
    bool isMouseRelease(int button) const {
        return (mouseButtonsRead_[button] & State::RELEASED_BIT) != State::ZERO;
    }

    /**
     * @param key one of `GLFW_KEY_*`
     * @return `true` if the given key is being held down.
     */
    bool isKeyDown(int key) const {
        return (keysRead_[key] & State::PERSISTENT_PRESSED_MASK) != State::ZERO;
    }

    /**
     * @param key the printed key symbol.
     * @return `true` if the given key is being held down.
     */
    bool isKeyDown(std::string key) const {
        if (keyMap_.count(key) == 0)
            return false;
        return isKeyDown(keyMap_.at(key));
    }

    /**
     * @param key one of `GLFW_KEY_*`. Note: This uses the physical position in the US layout.
     * @return `true` if the given key has been pressed down since the last frame.
     */
    bool isKeyPress(int key) const {
        return (keysRead_[key] & State::PRESSED_BIT) != State::ZERO;
    }

    /**
     * @param key one of `GLFW_KEY_*`
     * @return `true` if the given key has been released up since the last frame.
     */
    bool isKeyRelease(int key) const {
        return (keysRead_[key] & State::RELEASED_BIT) != State::ZERO;
    }

    void update();

    // sets a flag that will poll the true input state on the next update
    void invalidate() {
        stateInvalid_ = true;
    }

    void onKey(GLFWwindow *window, int key, int scancode, int action, int mods);
    void onCursorPos(GLFWwindow *window, double x, double y);
    void onMouseButton(GLFWwindow *window, int button, int action, int mods);
    void onScroll(GLFWwindow *window, double dx, double dy);

    static Input *init(GLFWwindow *window);
};