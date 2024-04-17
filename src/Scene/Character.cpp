
#include "Character.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
// include after ObjectLayer
#include <Jolt/Physics/Character/Character.h>

#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/fast_trigonometry.hpp>
#include <glm/gtx/transform.hpp>

#include "../Camera.h"
#include "../Controller/MainController.h"
#include "../Game.h"
#include "../Input.h"
#include "../Physics/Physics.h"
#include "../Physics/Shapes.h"
#include "../UI/Screens/Fade.h"
#include "../Util/Log.h"
#include "../Window.h"
#include "Objects/Checkpoint.h"

using namespace scene;

glm::vec2 quatToAzimuthElevation(const glm::quat& q);

CharacterController::CharacterController(SceneRef scene, Camera& camera) : Entity(scene), camera(camera) {
}

CharacterController::~CharacterController() {
    if (body_ != nullptr) {
        body_->RemoveFromPhysicsSystem();
    }
    delete body_;
}

void CharacterController::init() {
    JPH::Ref<JPH::CharacterSettings> settings = new JPH::CharacterSettings();
    settings->mMaxSlopeAngle = JPH::DegreesToRadians(45.0f);
    settings->mLayer = ph::Layers::MOVING;
    settings->mShape = new JPH::SphereShape(0.5f);
    settings->mFriction = 0.0f;
    settings->mGravityFactor = 0.0f;

    body_ = new JPH::Character(settings, JPH::RVec3(0.0, 1.5, 2.0), JPH::Quat::sIdentity(), 0, physics().system);
    body_->AddToPhysicsSystem(JPH::EActivation::Activate);
    cameraLerpStart_ = cameraLerpEnd_ = ph::convert(body_->GetPosition());

    physics().contactListener->RegisterCallback(
        body_->GetBodyID(),
        [this](ph::SensorContact contact) {
            if (!contact.persistent) this->onBodyContact_(contact);
        });
}

void CharacterController::onBodyContact_(ph::SensorContact& contact) {
    NodeRef contactNode = scene.byPhysicsBody(contact.other);
    // The contact node should always have physics
    if (!contactNode.hasPhysics()) {
        LOG_WARN("Contact node did not have physics?!?");
        return;
    }
    // can't collide with triggers
    if (contactNode.physics().hasTrigger()) return;

    if (!invulnerabilityTimer.isZero()) return;
    invulnerabilityTimer = 2.0;
    respawn_();
}

void CharacterController::respawn_() {
    MainController& controller = dynamic_cast<MainController&>(*game().controller);
    CheckpointEntity* last_checkpoint = controller.raceManager.getLastCheckpoint();
    if (last_checkpoint == nullptr) return;
    scene::TransformRef transform = last_checkpoint->respawnTransformation();
    glm::vec2 azimuth_elevation = quatToAzimuthElevation(transform.rotation());
    camera.angles = {azimuth_elevation.y, azimuth_elevation.x, 0};
    setPosition_(transform.position());
    controller.fader->fade(1.0f, 0.0f, 0.3f);
    noMoveTimer.set(0.3f);
}

void CharacterController::setPosition_(glm::vec3 pos) {
    camera.position = pos;
    body_->SetPosition(ph::convert(pos));
    cameraLerpStart_ = pos;
    cameraLerpEnd_ = pos;
}

void CharacterController::update() {
    Input& input = *game().input;
    if (input.isMouseReleased()) {
        velocity_ = glm::vec3(0);
        return;
    }

    invulnerabilityTimer.update(input.timeDelta());
    noMoveTimer.update(input.timeDelta());

    // Camera movement
    // yaw
    camera.angles.y -= input.mouseDelta().x * glm::radians(LOOK_SENSITIVITY);
    camera.angles.y = glm::wrapAngle(camera.angles.y);

    // pitch
    camera.angles.x -= input.mouseDelta().y * glm::radians(LOOK_SENSITIVITY);
    camera.angles.x = glm::clamp(camera.angles.x, -glm::half_pi<float>(), glm::half_pi<float>());

    // query the input GLFW_KEY_F for flying toggle
    if (input.isKeyPress(GLFW_KEY_F)) {
        isAutoMoveEnabled = !isAutoMoveEnabled;
    }

    if (isAutoMoveEnabled) {
        glm::vec3 moveDirection_ = camera.rotationMatrix() * glm::vec3(0, 0, -1);
        velocity_ = moveDirection_ * AUTO_MOVE_SPEED;
    } else
        velocity_ = glm::vec3{0, 0, 0};

    // Calculate movement input. Use the trick that in c++ we can substract booleans
    glm::vec3 move_input = {
        input.isKeyDown(GLFW_KEY_D) - input.isKeyDown(GLFW_KEY_A),
        input.isKeyDown(GLFW_KEY_SPACE) - input.isKeyDown(GLFW_KEY_LEFT_CONTROL),
        input.isKeyDown(GLFW_KEY_S) - input.isKeyDown(GLFW_KEY_W)};
    velocity_ += move_input * SPEED;
    if (input.isKeyDown(GLFW_KEY_LEFT_SHIFT)) {
        velocity_ *= SHIFT_SPEED_FACTOR;
    }

    if (!noMoveTimer.isZero()) {
        velocity_ = glm::vec3(0);
    }

    // Rotate the velocity vector towards look direction
    if (!isAutoMoveEnabled)
        velocity_ = glm::mat3(glm::rotate(glm::mat4(1.0), camera.angles.y, {0, 1, 0})) * velocity_;
    // velocity_ = glm::mat3(glm::rotate(glm::mat4(1.0), camera->angles.x, {1, 0, 0})) * velocity_;
    //  The calculated velocity is used later during the physics update.

    // Interpolate the camera position from the previous physics body position to the current one.
    // The physics body only moves at a fixed interval (60Hz) but the camera movement needs to be smooth.
    // That's why interpolation is needed, so the camera updates its position every frame.
    camera.position = glm::mix(cameraLerpStart_, cameraLerpEnd_, physics().partialTicks());
    camera.updateViewMatrix();
}

void CharacterController::prePhysicsUpdate() {
    cameraLerpStart_ = ph::convert(body_->GetPosition());
    body_->SetLinearVelocity(ph::convert(velocity_));
}

void CharacterController::postPhysicsUpdate() {
    cameraLerpEnd_ = ph::convert(body_->GetPosition());
}

// calculate azimuth and elevation from quaternion. (assuming -z forward)
glm::vec2 quatToAzimuthElevation(const glm::quat& q) {
    // Calculate elevation
    float sin_pitch = 2.0f * (q.x * q.w - q.z * q.y);
    float elevation = glm::asin(sin_pitch);

    // Calculate azimuth (handle gimbal lock near poles)
    float azimuth;
    if (glm::epsilonEqual(glm::abs(sin_pitch), 1.0f, glm::epsilon<float>())) {
        azimuth = 0.0f;
    } else {
        azimuth = glm::atan(q.y * q.w - q.z * q.x, 0.5f - q.x * q.x - q.y * q.y);
    }

    return glm::vec2(azimuth, elevation);
}