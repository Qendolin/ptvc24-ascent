#pragma once

#include <array>
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct Window;
struct GLFWwindow;

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
   public:
    typedef int CallbackRegistrationID;

    typedef std::function<void(float x, float y)> MousePosCallback;
    typedef std::function<void(int button, int action, int mods)> MouseButtonCallback;
    typedef std::function<void(float x, float y)> ScrollCallback;
    typedef std::function<void(int key, int scancode, int action, int mods)> KeyCallback;
    typedef std::function<void(unsigned int codepoint)> CharCallback;

   private:
    template <typename T>
    struct CallbackRegistration {
        CallbackRegistrationID id;
        T callback;
    };

    enum class State : uint8_t {
        Zero = 0,
        ReleasedBit = 0b001,
        PressedBit = 0b010,
        PersistentPressedBit = 0b100,
        ClearMask = static_cast<State>(~static_cast<uint8_t>(ReleasedBit | PressedBit)),
        PersistentPressedMask = PressedBit | PersistentPressedBit,
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

    const Window &window_;
    double timeRead_ = 0;
    float timeDelta_ = 0;
    bool mouseCaptured_ = false;
    glm::vec2 mousePosRead_ = {};
    glm::vec2 mousePosWrite_ = {};
    glm::vec2 mouseDelta_ = {};
    glm::vec2 scrollDeltaRead_ = {};
    glm::vec2 scrollDeltaWrite_ = {};
    std::array<State, 8> mouseButtonsRead_;
    std::array<State, 8> mouseButtonsWrite_;
    std::array<State, 349> keysRead_;
    std::array<State, 349> keysWrite_;
    std::unordered_map<std::string, int> keyMap_ = {};

    bool stateInvalid_ = true;

    int nextCallbackRegistrationId_ = 1;
    std::vector<CallbackRegistration<MousePosCallback>> mousePosCallbacks_;
    std::vector<CallbackRegistration<MouseButtonCallback>> mouseButtonCallbacks_;
    std::vector<CallbackRegistration<ScrollCallback>> scrollCallbacks_;
    std::vector<CallbackRegistration<KeyCallback>> keyCallbacks_;
    std::vector<CallbackRegistration<CharCallback>> charCallbacks_;

    /**
     * Polls every key and mouse button to ensure that the internal state is up to date.
     * Is called after invalidate.
     */
    void pollCurrentState_();

   public:
    Input(Window &window);
    ~Input();

    /**
     * @return the mouse position measured from top-left corner of the viewport
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
     * @return the time difference since the last frame, in seconds
     */
    const float timeDelta() { return timeDelta_; }
    /**
     * @return the time since GLFW was initialized, in seconds
     */
    const double time() { return timeRead_; }

    /**
     * @return `true` if the mouse is captured (aka. grabbed).
     */
    bool isMouseCaptured() {
        return mouseCaptured_;
    }

    void captureMouse();

    /**
     * @return `true` if the mouse is **not** captured (aka. grabbed).
     */
    bool isMouseReleased() {
        return !mouseCaptured_;
    }

    void releaseMouse();

    /**
     * @return `true` if the window is focused / selected by the user
    */
    bool isWindowFocused();

    /**
     * @param button one of `GLFW_MOUSE_BUTTON_*`
     * @return `true` if the given button is being held down.
     */
    bool isMouseDown(int button) const {
        return (mouseButtonsRead_[button] & State::PersistentPressedMask) != State::Zero;
    }

    /**
     * @param button one of `GLFW_MOUSE_BUTTON_*`
     * @return `true` if the given button has been pressed down since the last frame.
     */
    bool isMousePress(int button) const {
        return (mouseButtonsRead_[button] & State::PressedBit) != State::Zero;
    }

    /**
     * @param button one of `GLFW_MOUSE_BUTTON_*`
     * @return `true` if the given button has been pressed down since the last frame.
     */
    bool isMouseRelease(int button) const {
        return (mouseButtonsRead_[button] & State::ReleasedBit) != State::Zero;
    }

    /**
     * @param key one of `GLFW_KEY_*`
     * @return `true` if the given key is being held down.
     */
    bool isKeyDown(int key) const {
        return (keysRead_[key] & State::PersistentPressedMask) != State::Zero;
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
        return (keysRead_[key] & State::PressedBit) != State::Zero;
    }

    /**
     * @param key one of `GLFW_KEY_*`
     * @return `true` if the given key has been released up since the last frame.
     */
    bool isKeyRelease(int key) const {
        return (keysRead_[key] & State::ReleasedBit) != State::Zero;
    }

    /**
     * Register a mouse positon callback
     * @param x mouse x position in window coordinates
     * @param y mouse y position in window coordinates
     */
    CallbackRegistrationID addMousePosCallback(MousePosCallback callback) {
        int id = nextCallbackRegistrationId_++;
        mousePosCallbacks_.emplace_back(id, callback);
        return id;
    }

    /**
     * Register a mouse button callback
     * @param button one of `GLFW_MOUSE_BUTTON_*`
     * @param action one of `GLFW_PRESS` or `GLFW_RELEASE`
     * @param mods a bitfiled of `GLFW_MOD_*`
     */
    CallbackRegistrationID addMouseButtonCallback(MouseButtonCallback callback) {
        int id = nextCallbackRegistrationId_++;
        mouseButtonCallbacks_.emplace_back(id, callback);
        return id;
    }

    /**
     * Register a mouse wheel callback
     * @param x horizontal scroll delta
     * @param y vertical scroll delta
     */
    CallbackRegistrationID addScrollCallback(ScrollCallback callback) {
        int id = nextCallbackRegistrationId_++;
        scrollCallbacks_.emplace_back(id, callback);
        return id;
    }

    /**
     * Register a keyboard key callback
     * @param key one of `GLFW_KEY_*`
     * @param scancode the scancode of the key
     * @param action one of `GLFW_PRESS`, `GLFW_RELEASE` or `GLFW_REPEAT`
     * @param mods a bitfiled of `GLFW_MOD_*`
     */
    CallbackRegistrationID addKeyCallback(KeyCallback callback) {
        int id = nextCallbackRegistrationId_++;
        keyCallbacks_.emplace_back(id, callback);
        return id;
    }

    /**
     * Register a keyboard character callback
     * @param codepoint the charachter codepoint
     */
    CallbackRegistrationID addCharCallback(CharCallback callback) {
        int id = nextCallbackRegistrationId_++;
        charCallbacks_.emplace_back(id, callback);
        return id;
    }

    /**
     * Remove a callback given its registration id
     */
    void removeCallback(CallbackRegistrationID &registration);

    void update();

    // sets a flag that will poll the true input state on the next update
    void invalidate() {
        stateInvalid_ = true;
    }

    void onKey(GLFWwindow *window, int key, int scancode, int action, int mods);
    void onCursorPos(GLFWwindow *window, double x, double y);
    void onMouseButton(GLFWwindow *window, int button, int action, int mods);
    void onScroll(GLFWwindow *window, double dx, double dy);
    void onChar(GLFWwindow *window, unsigned int codepoint);

    static Input *init(GLFWwindow *window);
};