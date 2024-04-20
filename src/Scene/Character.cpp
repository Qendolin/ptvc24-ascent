
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

    respawn_();
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

    scene::TransformRef transform;

    CheckpointEntity* last_checkpoint = controller.raceManager.getLastCheckpoint();
    if (last_checkpoint != nullptr) {
        transform = last_checkpoint->respawnTransformation();
    } else {
        transform = scene.find(scene.root(), "PlayerSpawn").transform();
    }

    glm::vec2 azimuth_elevation = quatToAzimuthElevation(transform.rotation());
    camera.angles = {azimuth_elevation.y, azimuth_elevation.x, 0};
    setPosition_(transform.position());
    controller.fader->fade(1.0f, 0.0f, 0.3f);
    noMoveTimer.set(0.3f);
    velocity_ = transform.rotation() * glm::vec3(0, 0, -SPEED / 2);
}

void CharacterController::setPosition_(glm::vec3 pos) {
    camera.position = pos;
    body_->SetPosition(ph::convert(pos));
    cameraLerpStart_ = pos;
    cameraLerpEnd_ = pos;
}

void CharacterController::update(float time_delta) {
    Input& input = *game().input;
    if (input.isMouseReleased()) {
        velocity_ = glm::vec3(0);
        return;
    }

    invulnerabilityTimer.update(time_delta);
    noMoveTimer.update(time_delta);
    if (!noMoveTimer.isZero()) {
        return;
    }
    // Camera movement
    // yaw
    camera.angles.y -= input.mouseDelta().x * glm::radians(LOOK_SENSITIVITY);
    camera.angles.y = glm::wrapAngle(camera.angles.y);

    // pitch
    camera.angles.x -= input.mouseDelta().y * glm::radians(LOOK_SENSITIVITY);
    camera.angles.x = glm::clamp(camera.angles.x, -glm::half_pi<float>(), glm::half_pi<float>());

    velocityUpdate(input.timeDelta());

    // if (!noMoveTimer.isZero()) {
    //     velocity_ = glm::vec3(0);
    // }

    //  The calculated velocity is used later during the physics update.

    // Interpolate the camera position from the previous physics body position to the current one.
    // The physics body only moves at a fixed interval (60Hz) but the camera movement needs to be smooth.
    // That's why interpolation is needed, so the camera updates its position every frame.
    camera.position = glm::mix(cameraLerpStart_, cameraLerpEnd_, physics().partialTicks());
}

void CharacterController::prePhysicsUpdate() {
    if (!noMoveTimer.isZero()) {
        body_->SetLinearVelocity(ph::convert(glm::vec3{0, 0, 0}));
        return;
    }
    cameraLerpStart_ = ph::convert(body_->GetPosition());
    body_->SetLinearVelocity(ph::convert(velocity_));
}

void CharacterController::postPhysicsUpdate() {
    cameraLerpEnd_ = ph::convert(body_->GetPosition());
}

void CharacterController::velocityUpdate(float deltatime) {
    glm::vec3 lookAt = camera.rotationMatrix() * glm::vec3(0, 0, -1);
    float pitchCos = glm::cos(-camera.angles.x);
    float pitchSin = glm::sin(-camera.angles.x);
    float hLook = pitchCos;
    float sqrPitchCos = pitchCos * pitchCos;
    velocity_.y += 0.5f * (sqrPitchCos * 0.75f - 1) * deltatime * SPEED;
    float hvel = glm::sqrt(velocity_.x * velocity_.x + velocity_.z * velocity_.z);

    if (velocity_.y < 0 && hLook > 0) {
        float yacc = velocity_.y * -0.1f * sqrPitchCos * deltatime * SPEED;
        velocity_.y += yacc;
        velocity_.x += lookAt.x * yacc / hLook;
        velocity_.z += lookAt.z * yacc / hLook;
    }
    if (camera.angles.x > 0 && hLook > 0) {
        float yacc = hvel * -pitchSin * 0.04f * deltatime * SPEED;
        velocity_.y += yacc * 3.5f;
        velocity_.x -= lookAt.x * yacc / hLook;
        velocity_.z -= lookAt.z * yacc / hLook;
    }
    if (hLook > 0) {
        velocity_.x += (lookAt.x / hLook * hvel - velocity_.x) * 0.1f * deltatime * SPEED;
        velocity_.z += (lookAt.z / hLook * hvel - velocity_.z) * 0.1f * deltatime * SPEED;
    }

    velocity_.x *= 1.0f - 0.01f * deltatime;
    velocity_.y *= 1.0f - 0.02f * deltatime;
    velocity_.z *= 1.0f - 0.01f * deltatime;
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
