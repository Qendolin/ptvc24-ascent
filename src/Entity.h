#pragma once

#include <Jolt/Jolt.h>

#include <glm/glm.hpp>

#include "Camera.h"
#include "Physics/Physics.h"

// After Physics.h
#include <Jolt/Physics/Character/Character.h>

// Entity represents objects that need to process game logic
class Entity {
   public:
    virtual ~Entity() {}

    // Called at scene creation
    virtual void init() = 0;

    // Called every frame, not before physics update
    virtual void update() = 0;

    // Called just before every physics step
    virtual void prePhysicsUpdate() {}

    // Called just after every physics step
    virtual void postPhysicsUpdate() {}
};

// The character controller handles movement and mouse look.
// It links the character physics body to the camera.
class CharacterController : public Entity {
   private:
    // Walking speed in m/s
    inline static const float SPEED = 2.5;
    // Speed multiplier when shift is held down
    inline static const float SHIFT_SPEED_FACTOR = 5.0;
    // How much the camera turns when moving the mouse. The unit is Degrees / Pixel.
    inline static const float LOOK_SENSITIVITY = 0.333;

    JPH::Character* body_ = nullptr;
    glm::vec3 velocity_ = {};

    // The start and end position of the camera interpolation
    glm::vec3 cameraLerpStart_ = {};
    glm::vec3 cameraLerpEnd_ = {};

   public:
    Camera* camera = nullptr;

    CharacterController(Camera* camera);

    ~CharacterController();

    virtual void init() override;

    virtual void update() override;

    virtual void prePhysicsUpdate() override;

    virtual void postPhysicsUpdate() override;
};