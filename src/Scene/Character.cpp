
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

#include "../Audio/Assets.h"
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

CharacterEntity::CharacterEntity(SceneRef scene, NodeRef node, Camera& camera) : NodeEntity(scene, node), camera(camera) {
    auto& windSound = game().audio->assets->wind;
    windSoundInstanceLeft_ = std::unique_ptr<SoundInstance2d>(windSound.play2d(0, -0.5));
    windSoundInstanceRight_ = std::unique_ptr<SoundInstance2d>(windSound.play2d(0, 0.5));
    windSoundInstanceLeft_->seek(windSound.duration() * 0.5);
    boostSoundInstance_ = std::unique_ptr<SoundInstance2d>(game().audio->assets->boost.play2d(0, 0.0));
}

CharacterEntity::~CharacterEntity() {
    if (body_ != nullptr) {
        body_->RemoveFromPhysicsSystem();
    }
    delete body_;
    if (kinematicBody_ != nullptr) {
        physics().interface().RemoveBody(kinematicBody_->GetID());
        physics().interface().DestroyBody(kinematicBody_->GetID());
    }
    // I'm not sure, but I guess Jolt already deletes it when I call DestroyBody
    // delete kinematicBody_;
}

void CharacterEntity::init() {
    base.addTag("player");

    JPH::Ref<JPH::CharacterSettings> character_settings = new JPH::CharacterSettings();
    character_settings->mMaxSlopeAngle = JPH::DegreesToRadians(45.0f);
    character_settings->mLayer = ph::Layers::PLAYER;
    character_settings->mShape = new JPH::SphereShape(0.25f);
    character_settings->mFriction = 0.0f;
    character_settings->mGravityFactor = 0.0f;

    body_ = new JPH::Character(character_settings, JPH::RVec3(0.0, 0.0, 0.0), JPH::Quat::sIdentity(), 0, physics().system);
    body_->AddToPhysicsSystem(JPH::EActivation::Activate);
    cameraLerpStart_ = cameraLerpEnd_ = ph::convert(body_->GetPosition());

    // the "hurtbox" is much larger than the hitbox
    JPH::BodyCreationSettings kinematic_body_settings(new JPH::SphereShape(1.0f), JPH::RVec3(0.0, 0.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Kinematic, ph::Layers::PLAYER);
    kinematic_body_settings.mMotionQuality = JPH::EMotionQuality::LinearCast;
    kinematicBody_ = physics().interface().CreateBody(kinematic_body_settings);
    physics().interface().AddBody(kinematicBody_->GetID(), JPH::EActivation::Activate);

    physics().contactListener->RegisterCallback(
        body_->GetBodyID(),
        [this](ph::SensorContact contact) {
            if (!contact.persistent) this->onBodyContact_(contact);
        });
}

void CharacterEntity::onBodyContact_(ph::SensorContact& contact) {
    NodeRef contactNode = scene.byPhysicsBody(contact.other);
    if (contactNode.isInvalid()) {
        LOG_WARN("Contact node was invalid.");
        return;
    }
    // The contact node should always have physics
    if (!contactNode.hasPhysics()) {
        LOG_WARN("Contact node did not have physics?!?");
        return;
    }
    // can't collide with triggers
    if (contactNode.physics().hasTrigger()) return;

    if (contactNode.hasTag("non_killing")) return;

    if (!respawnInvulnerability.isZero()) return;
    game().audio->assets->thump.play2dEvent(0.5, 0);
    respawn();
}

void CharacterEntity::respawn() {
    MainController& controller = dynamic_cast<MainController&>(*game().controller);

    RaceManager::RespawnPoint respawn_point = controller.raceManager.respawnPoint();

    glm::quat rotation = glm::quat_cast(respawn_point.transform);
    glm::vec2 azimuth_elevation = quatToAzimuthElevation(rotation);
    camera.angles = {azimuth_elevation.y, azimuth_elevation.x, 0};

    setPosition_(respawn_point.transform[3]);
    controller.fader->fade(1.5f, 0.0f, RESPAWN_TIME);
    respawnFreeze = RESPAWN_TIME;
    respawnInvulnerability = INVULNERABILITY_TIME;

    float speed = std::max(RESPAWN_SPEED_MINIMUM, respawn_point.speed * RESPAWN_SPEED_FACTOR);
    velocity_ = rotation * glm::vec3(0, 0, -speed);

    boostMeter_ = std::max(respawn_point.boostMeter, RESPAWN_BOOST_MINIMUM);
}

void CharacterEntity::setPosition_(glm::vec3 pos) {
    camera.position = pos;
    body_->SetPosition(ph::convert(pos));
    physics().interface().SetPosition(kinematicBody_->GetID(), ph::convert(pos), JPH::EActivation::Activate);
    cameraLerpStart_ = pos;
    cameraLerpEnd_ = pos;
}

void CharacterEntity::terminate() {
    this->windSoundInstanceLeft_->stop();
    this->windSoundInstanceRight_->stop();
    this->boostSoundInstance_->stop();
}

void CharacterEntity::update(float time_delta) {
    if (!enabled) return;
    Input& input = *game().input;
    Settings settings = game().settings.get();

    respawnInvulnerability.update(time_delta);
    respawnFreeze.update(time_delta);
    if (isFrozen_()) {
        windSoundInstanceLeft_->setVolume(0);
        windSoundInstanceRight_->setVolume(0);
        boostSoundInstance_->setVolume(0);
        return;
    }
    // Camera movement
    // yaw
    camera.angles.y -= input.mouseDelta().x * glm::radians(settings.lookSensitivity);
    camera.angles.y = glm::wrapAngle(camera.angles.y);

    // pitch
    camera.angles.x -= input.mouseDelta().y * glm::radians(settings.lookSensitivity);
    camera.angles.x = glm::clamp(camera.angles.x, -glm::half_pi<float>(), glm::half_pi<float>());

    // Interpolate the camera position from the previous physics body position to the current one.
    // The physics body only moves at a fixed interval (60Hz) but the camera movement needs to be smooth.
    // That's why interpolation is needed, so the camera updates its position every frame.
    camera.position = glm::mix(cameraLerpStart_, cameraLerpEnd_, physics().partialTicks());

    if (input.isKeyDown(GLFW_KEY_R)) {
        respawn();
    }

    if (input.isKeyDown(GLFW_KEY_SPACE) || input.isKeyDown(GLFW_KEY_S)) {
        breakFlag_ = true;
    }

    if (input.isKeyDown(GLFW_KEY_LEFT_SHIFT) || input.isKeyDown(GLFW_KEY_W)) {
        boostFlag_ = true;

    } else {
        boostMeter_ = std::min(boostMeter_ + BOOST_REGEN * time_delta, 1.0f);
    }

    if (boostFlag_ && boostMeter_ > 0) {
        boostDynamicFov_ += BOOST_DYN_FOV_CHANGE * time_delta;
        if (!boostSoundPlaying_) {
            boostSoundPlaying_ = true;
            boostSoundInstance_->seek(0.0);
        }
        boostSoundInstance_->setVolume(1.0);
    } else {
        boostDynamicFov_ -= BOOST_DYN_FOV_CHANGE * time_delta;
        boostSoundInstance_->setVolume(0.0);
        boostSoundPlaying_ = false;
    }
    boostDynamicFov_ = std::clamp<float>(boostDynamicFov_, 0, BOOST_DYN_FOV_MAX);

    camera.setFov(glm::radians(settings.fov + boostDynamicFov_));

    float speed = glm::length(velocity_);
    float volume = glm::pow(glm::clamp(speed / 40.0f, 0.0f, 1.0f), 3.0f);
    windSoundInstanceLeft_->setVolume(volume * 1.25f);
    windSoundInstanceRight_->setVolume(volume * 1.25f);
}

void CharacterEntity::prePhysicsUpdate() {
    if (isFrozen_() || !enabled) {
        body_->SetLinearVelocity(ph::convert(glm::vec3{0, 0, 0}));
        return;
    }

    // the vlocity update has to use a fixed time step, else it becomes unstable
    velocity_ = calculateVelocity_(ph::Physics::UPDATE_INTERVAL);

    cameraLerpStart_ = ph::convert(body_->GetPosition());
    body_->SetLinearVelocity(ph::convert(velocity_));
}

void CharacterEntity::postPhysicsUpdate() {
    if (!enabled) return;

    cameraLerpEnd_ = ph::convert(body_->GetPosition());
    // move kinematic body to character body
    physics().interface().MoveKinematic(kinematicBody_->GetID(), body_->GetPosition(), body_->GetRotation(), ph::Physics::UPDATE_INTERVAL);
}

glm::vec3 CharacterEntity::calculateVelocity_(float time_delta) {
    glm::vec3 looking = camera.rotationMatrix() * glm::vec3(0, 0, -1);

    const glm::vec3 velocity = velocity_;
    glm::vec3 result = velocity;

    float pitch_cos = glm::cos(camera.angles.x);  // up, down = 0, horizontal = 1
    float pitch_sin = glm::sin(camera.angles.x);  // down = -1, horizontal = 0, up = 1
    float pitch_cos_sqr = pitch_cos * pitch_cos;
    float horizontal_speed = glm::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);

    if (boostFlag_) {
        boostFlag_ = false;
        float boost_want = BOOST_CONSUME * time_delta;
        float boost_used = std::min(boost_want, boostMeter_);
        boostMeter_ -= boost_used;
        float factor = boost_used / boost_want;  // 1, execpt if meter is empty
        result += factor * looking * BOOST_ACCELERATION * time_delta;
    }

    // Looking straight ahead cancles 75% of gravity due to aerodynamic drag
    float down_accel = (pitch_cos_sqr * 0.75f - 1) * GRAVITY_ACCELERATION;
    result.y += down_accel * time_delta;

    // Looking straight up or down would divide by zero
    if (pitch_cos > glm::epsilon<float>()) {
        // convert vertical to horizontal velocity
        if (velocity.y < 0) {
            float lift = velocity.y * -0.125f * pitch_cos_sqr * time_delta * SPEED;
            result.y += lift;
            result.x += looking.x * 1.1f * lift / pitch_cos;
            result.z += looking.z * 1.1f * lift / pitch_cos;
        }
        // convert horizontal to vertical velocity
        if (camera.angles.x > 0) {
            float lift = horizontal_speed * pitch_sin * 0.125f * time_delta * SPEED;
            result.y += lift;
            result.x -= looking.x * 0.9f * lift / pitch_cos;
            result.z -= looking.z * 0.9f * lift / pitch_cos;
        }

        // steering / turning
        result.x += (looking.x / pitch_cos * horizontal_speed - velocity.x) * TURN_FACTOR * time_delta;
        result.z += (looking.z / pitch_cos * horizontal_speed - velocity.z) * TURN_FACTOR * time_delta;
    }

    float break_drag = 0.02f;
    if (breakFlag_) {
        break_drag = BREAK_FACTOR;
        breakFlag_ = false;
    }

    // air drag
    result.x *= 1 - break_drag * time_delta;
    result.y *= 1 - 2 * break_drag * time_delta;
    result.z *= 1 - break_drag * time_delta;

    return result;
}

// calculate azimuth and elevation from quaternion. (assuming -z forward)
static glm::vec2 quatToAzimuthElevation(const glm::quat& q) {
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

JPH::BodyID CharacterEntity::body() const {
    return body_->GetBodyID();
}