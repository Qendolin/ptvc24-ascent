#include "Entity.h"

#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/fast_trigonometry.hpp>
#include <glm/gtx/transform.hpp>

#include "Game.h"

CharacterController::CharacterController(Camera* camera) : camera(camera) {
}

CharacterController::~CharacterController() {
    PH::Physics* physics = Game::instance->physics;
    if (body_ != nullptr) {
        body_->RemoveFromPhysicsSystem();
        delete body_;
        body_ = nullptr;
    }
}

void CharacterController::init() {
    JPH::Ref<JPH::CharacterSettings> settings = new JPH::CharacterSettings();
    settings->mMaxSlopeAngle = JPH::DegreesToRadians(45.0f);
    settings->mLayer = PH::Layers::MOVING;
    settings->mShape = new JPH::SphereShape(0.5f);
    settings->mFriction = 0.0f;
    settings->mGravityFactor = 0.0f;

    PH::Physics* physics = Game::instance->physics;

    body_ = new JPH::Character(settings, JPH::RVec3(0.0, 1.5, 2.0), JPH::Quat::sIdentity(), 0, physics->system);
    body_->AddToPhysicsSystem(JPH::EActivation::Activate);
    cameraLerpStart_ = cameraLerpEnd_ = PH::convert(body_->GetPosition());
}

void CharacterController::update() {
    if (camera == nullptr) {
        return;
    }

    Input* input = Game::instance->input;

    // Camera movement
    // yaw
    camera->angles.y -= input->mouseDelta().x * 0.003f;
    camera->angles.y = glm::wrapAngle(camera->angles.y);
    // pitch
    camera->angles.x -= input->mouseDelta().y * 0.003f;
    camera->angles.x = glm::clamp(camera->angles.x, -glm::half_pi<float>(), glm::half_pi<float>());

    // Calculate movement input. Use the trick that in c++ we can substract booleans
    glm::vec3 move_input = {
        input->isKeyDown(GLFW_KEY_D) - input->isKeyDown(GLFW_KEY_A),
        input->isKeyDown(GLFW_KEY_SPACE) - input->isKeyDown(GLFW_KEY_LEFT_CONTROL),
        input->isKeyDown(GLFW_KEY_S) - input->isKeyDown(GLFW_KEY_W)};
    velocity_ = move_input * SPEED;

    // Rotate the velocity vector towards look direction
    velocity_ = glm::mat3(glm::rotate(glm::mat4(1.0), camera->angles.y, {0, 1, 0})) * velocity_;
    if (input->isKeyDown(GLFW_KEY_LEFT_SHIFT)) {
        velocity_ *= SHIFT_SPEED_FACTOR;
    }

    camera->position = glm::mix(cameraLerpStart_, cameraLerpEnd_, Game::instance->physics->partialTicks());
    camera->updateViewMatrix();
}

void CharacterController::prePhysicsUpdate() {
    cameraLerpStart_ = PH::convert(body_->GetPosition());
    body_->SetLinearVelocity(PH::convert(velocity_));
}

void CharacterController::postPhysicsUpdate() {
    cameraLerpEnd_ = PH::convert(body_->GetPosition());
}
