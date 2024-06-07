#include "Crate.h"

#include "../../Game.h"
#include "../../Physics/Physics.h"
#include "../../Tween.h"

using namespace scene;

void CrateEntity::init() {
    colliderRef = base.find("Phys.Crate.Box");
    meshRef = base.find("Crate.Mesh");
    JPH::BodyID body = colliderRef.physics().body();
    posLerpStart = meshRef.transform().position();
    posLerpEnd = posLerpStart;
    rotLerpStart = meshRef.transform().rotation();
    rotLerpEnd = rotLerpStart;
}

void CrateEntity::update(float time_delta) {
    JPH::BodyID body = colliderRef.physics().body();
    meshRef.transform().setPosition(glm::mix(posLerpStart, posLerpEnd, physics().partialTicks()));
    meshRef.transform().setRotation(glm::slerp(rotLerpStart, rotLerpEnd, physics().partialTicks()));
    meshRef.graphics().setTransformFromNode();
}

void CrateEntity::prePhysicsUpdate() {
    JPH::BodyID body = colliderRef.physics().body();
    auto& interface = physics().interface();
    posLerpStart = ph::convert(interface.GetPosition(body));
    rotLerpStart = ph::convert(interface.GetRotation(body));
}

void CrateEntity::postPhysicsUpdate() {
    JPH::BodyID body = colliderRef.physics().body();
    auto& interface = physics().interface();
    posLerpEnd = ph::convert(interface.GetPosition(body));
    rotLerpEnd = ph::convert(interface.GetRotation(body));
}
