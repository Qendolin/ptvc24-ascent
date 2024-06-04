#include "BarrierObstacle.h"

#include "../../Debug/Direct.h"
#include "../../Game.h"
#include "../../Physics/Physics.h"
#include "../../Tween.h"

using namespace scene;

void BarrierObstacleEntity::init() {
    fromRef = base.find("*.MoveFrom");
    toRef = base.find("*.MoveTo");

    colliderRef = base.find("Phys.*.BoxCollider");
    meshRef = base.find("*.Mesh");

    float cycle_time = base.prop<float>("cycle_time");

    glm::vec3 from_pos = fromRef.transform().position();
    glm::vec3 to_pos = toRef.transform().position();

    tween = tweeny::from(from_pos.x, from_pos.y, from_pos.z)
                .to(to_pos.x, to_pos.y, to_pos.z)
                .during(cycle_time * 1000);
}

void BarrierObstacleEntity::update(float time_delta) {
    game().tween->step(tween);
    if (tween.progress() == 0.0) {
        tween.forward();
    }
    if (tween.progress() == 1.0) {
        tween.backward();
    }
    auto position = tween.peek();

    meshRef.transform().setPosition(position[0], position[1], position[2]);
    meshRef.graphics().setTransformFromNode();
}

void BarrierObstacleEntity::prePhysicsUpdate() {
    auto position = tween.peek();

    JPH::BodyID body = colliderRef.physics().body();
    // TODO: May want to use MoveKinematic
    physics().interface().SetPosition(body, {position[0], position[1], position[2]}, JPH::EActivation::Activate);
}

void BarrierObstacleEntity::debugDraw() {
    DirectBuffer& dd = *Game::get().directDraw;
    dd.unshaded();
    dd.stroke(0.1f);
    dd.color(0, 0, 0);
    dd.line(fromRef.transform().position(), toRef.transform().position());
}