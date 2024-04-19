#include "../Util/Timer.h"
#include "Entity.h"

#pragma region ForwardDecl
class Camera;
namespace JPH {
class Character;
}
namespace ph {
struct SensorContact;
}
#pragma endregion

// The character controller handles movement and mouse look.
// It links the character physics body to the camera.
class CharacterController : public scene::Entity {
   private:
    // Walking speed in m/s
    inline static const float SPEED = 2.5f;
    // Speed multiplier when shift is held down
    inline static const float SHIFT_SPEED_FACTOR = 5.0f;
    // How much the camera turns when moving the mouse. The unit is Degrees / Pixel.
    inline static const float LOOK_SENSITIVITY = 0.333f;
    // AutoMove Speed
    inline static const float AUTO_MOVE_SPEED = 20.0f;
    // query flying toggle
    bool isAutoMoveEnabled = false;
    Timer invulnerabilityTimer;
    Timer noMoveTimer;

    JPH::Character* body_ = nullptr;
    glm::vec3 velocity_ = {};

    // The start and end position of the camera interpolation
    glm::vec3 cameraLerpStart_ = {};
    glm::vec3 cameraLerpEnd_ = {};

    // called as a callback by the physics engine
    void onBodyContact_(ph::SensorContact& contact);

    // respawn the character at the last checkpoint
    void respawn_();

    void setPosition_(glm::vec3 pos);

   public:
    Camera& camera;

    CharacterController(scene::SceneRef scene, Camera& camera);

    virtual ~CharacterController();

    void init() override;

    void update(float time_delta) override;

    void prePhysicsUpdate() override;

    void postPhysicsUpdate() override;

    void velocityUpdate(float deltaTime);

    void setPosition_(glm::vec3 pos);
};