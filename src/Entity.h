#pragma once

#include <glm/glm.hpp>

#include "Camera.h"
#include "Physics/Physics.h"

// Entity represents objects in the world that need to process game logic
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

class CharacterController : public Entity {
   private:
    inline static const float SPEED = 2.5;
    inline static const float SHIFT_SPEED_FACTOR = 5.0;

    JPH::Character* body_ = nullptr;
    glm::vec3 velocity_ = {};
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