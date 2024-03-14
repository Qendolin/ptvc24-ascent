#pragma once

#include <GLFW/glfw3.h>

#include <array>
#include <glm/glm.hpp>
#include <iterator>
#include <memory>
#include <vector>

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

    friend constexpr inline State operator|(State lhs, State rhs);
    friend constexpr inline State operator&(State lhs, State rhs);
    friend constexpr inline State &operator|=(State &lhs, State rhs);
    friend constexpr inline State &operator&=(State &lhs, State rhs);

    static Input *instance_;

    float prevTime_ = 0;
    float time_ = 0;
    float timeDelta_ = 0;
    glm::vec2 mousePosRead_ = {};
    glm::vec2 mousePosWrite_ = {};
    glm::vec2 mouseDelta_ = {};
    glm::vec2 scrollDeltaRead_ = {};
    glm::vec2 scrollDeltaWrite_ = {};
    std::array<State, GLFW_MOUSE_BUTTON_LAST + 1> mouseButtonsRead_;
    std::array<State, GLFW_MOUSE_BUTTON_LAST + 1> mouseButtonsWrite_;
    std::array<State, GLFW_KEY_LAST + 1> keysRead_;
    std::array<State, GLFW_KEY_LAST + 1> keysWrite_;

   public:
    Input();
    ~Input();

    /**
     * @return the mouse position
     */
    const glm::vec2 mousePos() { return mousePosWrite_; }
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
     * @param button one of `GLFW_MOUSE_BUTTON_*`
     * @return `true` if the given button is being held down.
     */
    const bool isMouseDown(int button) {
        return (mouseButtonsRead_[button] & State::PERSISTENT_PRESSED_MASK) != State::ZERO;
    }

    /**
     * @param button one of `GLFW_MOUSE_BUTTON_*`
     * @return `true` if the given button has been pressed down since the last frame.
     */
    const bool isMousePress(int button) {
        return (mouseButtonsRead_[button] & State::PRESSED_BIT) != State::ZERO;
    }

    /**
     * @param button one of `GLFW_MOUSE_BUTTON_*`
     * @return `true` if the given button has been pressed down since the last frame.
     */
    const bool isMouseRelease(int button) {
        return (mouseButtonsRead_[button] & State::RELEASED_BIT) != State::ZERO;
    }

    /**
     * @param key one of `GLFW_KEY_*`
     * @return `true` if the given key is being held down.
     */
    const bool isKeyDown(int key) {
        return (keysRead_[key] & State::PERSISTENT_PRESSED_MASK) != State::ZERO;
    }

    /**
     * @param key one of `GLFW_KEY_*`
     * @return `true` if the given key has been pressed down since the last frame.
     */
    const bool isKeyPress(int key) {
        return (keysRead_[key] & State::PRESSED_BIT) != State::ZERO;
    }

    /**
     * @param key one of `GLFW_KEY_*`
     * @return `true` if the given key has been released up since the last frame.
     */
    const bool isKeyRelease(int key) {
        return (keysRead_[key] & State::RELEASED_BIT) != State::ZERO;
    }

    void update();

    void onKey(GLFWwindow *window, int key, int scancode, int action, int mods);
    void onCursorPos(GLFWwindow *window, double x, double y);
    void onMouseButton(GLFWwindow *window, int button, int action, int mods);
    void onScroll(GLFWwindow *window, double dx, double dy);

    static Input *instance();
    static Input *init(GLFWwindow *window);
};