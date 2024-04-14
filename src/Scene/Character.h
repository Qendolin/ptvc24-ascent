#include "Entity.h"

// forward declarations
class Camera;
namespace JPH {
class Character;
}

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

    JPH::Character* body_ = nullptr;
    glm::vec3 velocity_ = {};

    // The start and end position of the camera interpolation
    glm::vec3 cameraLerpStart_ = {};
    glm::vec3 cameraLerpEnd_ = {};

   public:
    Camera& camera;

    CharacterController(Camera& camera);

    virtual ~CharacterController();

    void init() override;

    void update() override;

    void prePhysicsUpdate() override;

    void postPhysicsUpdate() override;
};