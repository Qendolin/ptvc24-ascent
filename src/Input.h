#pragma once

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <iterator>
#include <memory>
#include <vector>

class Input {
   private:
    static Input *instance_;

    float prevTime_;
    float time_;
    float timeDelta_;
    glm::vec2 mousePrevPos_;
    glm::vec2 mousePos_;
    glm::vec2 mouseDelta_;
    glm::vec2 scrollDelta_;
    glm::vec2 scrollNextDelta_;
    bool mousePrevButtons_[GLFW_MOUSE_BUTTON_LAST + 1];
    bool mouseButtons_[GLFW_MOUSE_BUTTON_LAST + 1];
    bool keysPrev_[GLFW_KEY_LAST + 1];
    bool keys_[GLFW_KEY_LAST + 1];

   public:
    /**
     * @return the mouse position
     */
    const glm::vec2 mousePos() { return mousePos_; }
    /**
     * @return the mouse position difference since the last frame
     */
    const glm::vec2 mouseDelta() { return mouseDelta_; }
    /**
     * @return the scroll wheel position difference since the last frame
     */
    const glm::vec2 scrollDelta() { return scrollDelta_; }
    /**
     * @return the time difference since the last frame
     */
    const float timeDelta() { return timeDelta_; }

    /**
     * @param button one of `GLFW_MOUSE_BUTTON_*`
     * @return `true` if the given button is being held down.
     */
    const bool isMouseDown(int button) {
        return mouseButtons_[button];
    }

    /**
     * @param button one of `GLFW_MOUSE_BUTTON_*`
     * @return `true` if the given button has been pressed down since the last frame.
     */
    const bool isMousePress(int button) {
        return mouseButtons_[button] && !mousePrevButtons_[button];
    }

    /**
     * @param key one of `GLFW_KEY_*`
     * @return `true` if the given key is being held down.
     */
    const bool isKeyDown(int key) {
        return keys_[key];
    }

    /**
     * @param key one of `GLFW_KEY_*`
     * @return `true` if the given key has been pressed down since the last frame.
     */
    const bool isKeyPress(int key) {
        return keys_[key] && !keysPrev_[key];
    }

    /**
     * @param key one of `GLFW_KEY_*`
     * @return `true` if the given key has been released up since the last frame.
     */
    const bool isKeyRelease(int key) {
        return !keys_[key] && keysPrev_[key];
    }

    void update();

    void onKey(GLFWwindow *window, int key, int scancode, int action, int mods);
    void onCursorPos(GLFWwindow *window, double x, double y);
    void onMouseButton(GLFWwindow *window, int button, int action, int mods);
    void onScroll(GLFWwindow *window, double dx, double dy);

    static Input *instance();
    static Input *init(GLFWwindow *window);
};