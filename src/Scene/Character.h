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
class Sound;
class SoundInstance2d;
#pragma endregion

// The character controller handles movement and mouse look.
// It links the character physics body to the camera.
class CharacterEntity : public scene::Entity {
   private:
    // Flying speed factor
    inline static const float SPEED = 10.0f;
    // Controls the break decceleration
    inline static const float BREAK_FACTOR = 1.0f;
    // Controls the fall speed
    inline static const float GRAVITY_ACCELERATION = 9.81f;
    inline static const float BOOST_ACCELERATION = 10;
    // Consumption per second when boost is active
    inline static const float BOOST_CONSUME = 1.0f / 2.0f;
    // Regeneration per second when boost is inactive
    inline static const float BOOST_REGEN = 1.0f / 10.0f;
    inline static const float BOOST_DYN_FOV_MAX = 20;
    inline static const float BOOST_DYN_FOV_CHANGE = 30;
    // Controls how quicky the velocity matches the horizontal look direction
    inline static const float TURN_FACTOR = 6.0f;
    inline static const float RESPAWN_TIME = 0.65f;

    Timer respawnInvulnerability;
    Timer respawnFreeze;

    JPH::Character* body_ = nullptr;
    glm::vec3 velocity_ = {};
    // Set when should apply air break
    bool breakFlag_ = false;
    // Set when should apply boost
    bool boostFlag_ = false;
    // how much boost is available
    float boostMeter_ = 1.0;
    float boostDynamicFov_ = 0.0;

    bool frozen_ = false;
    ;

    // The start and end position of the camera interpolation
    glm::vec3 cameraLerpStart_ = {};
    glm::vec3 cameraLerpEnd_ = {};

    std::unique_ptr<Sound> windSound_;
    std::unique_ptr<SoundInstance2d> windSoundInstanceLeft_;
    std::unique_ptr<SoundInstance2d> windSoundInstanceRight_;

    // called as a callback by the physics engine
    void onBodyContact_(ph::SensorContact& contact);

    void setPosition_(glm::vec3 pos);

    bool isFrozen_() {
        return frozen_ || !respawnFreeze.isZero();
    }

    // called in physics update
    glm::vec3 calculateVelocity_(float time_delta);

   public:
    Camera& camera;
    bool enabled = true;

    CharacterEntity(scene::SceneRef scene, Camera& camera);

    virtual ~CharacterEntity();

    // respawn the character at the last checkpoint
    void respawn();

    void init() override;

    void update(float time_delta) override;

    void prePhysicsUpdate() override;

    void postPhysicsUpdate() override;

    glm::vec3 velocity() const {
        return velocity_;
    }

    void setFrozen(bool frozen) {
        frozen_ = frozen;
    }

    float boostMeter() const {
        return boostMeter_;
    }
};