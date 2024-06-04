#include "MillObstacle.h"

#include <glm/gtx/fast_trigonometry.hpp>

#include "../../Game.h"
#include "../../Physics/Physics.h"
#include "../../Tween.h"

using namespace scene;

void MillObstacleEntity::init() {
    colliderRef = base.find("Phys.MillBlades.Collider");
    meshRef = base.find("MillBlades.Mesh");
    initialBladeQuat = meshRef.transform().rotation();

    cycleTime = base.prop<float>("cycle_time");
}

void MillObstacleEntity::update(float time_delta) {
    bladeAngle += time_delta * glm::two_pi<float>() / cycleTime;
    bladeAngle = glm::wrapAngle(bladeAngle);

    glm::quat rotation_local = glm::angleAxis(bladeAngle, glm::vec3(0, 0, 1));
    glm::quat rotation_world = initialBladeQuat * rotation_local;

    meshRef.transform().setRotation(rotation_world);
    meshRef.graphics().setTransformFromNode();
}

void MillObstacleEntity::prePhysicsUpdate() {
    JPH::BodyID body = colliderRef.physics().body();
    physics().interface().SetRotation(body, ph::convert(meshRef.transform().rotation()), JPH::EActivation::Activate);
}
