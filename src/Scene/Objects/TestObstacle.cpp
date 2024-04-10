#include "TestObstacle.h"

#include "../../Game.h"

using namespace scene;

void TestObstacleEntity::init() {
    from = base.find("*.MoveFrom");
    to = base.find("*.MoveTo");

    collider = base.find("Phys.*.BoxCollider");
    mesh = base.find("*.Mesh");

    float cycle_time = base.prop<float>("cycle_time");

    glm::vec3 from_pos = from.transform().position();
    glm::vec3 to_pos = to.transform().position();

    tween = tweeny::from(from_pos.x, from_pos.y, from_pos.z)
                .to(to_pos.x, to_pos.y, to_pos.z)
                .during(cycle_time * 1000);
}

void TestObstacleEntity::update() {
    Game::instance->tween.step(tween);
    if (tween.progress() == 0.0) {
        tween.forward();
    }
    if (tween.progress() == 1.0) {
        tween.backward();
    }
    auto position = tween.peek();

    mesh.transform().setPosition(position[0], position[1], position[2]);
    mesh.graphics().setTransformFromNode();
}

void TestObstacleEntity::prePhysicsUpdate() {
    auto position = tween.peek();

    JPH::BodyID body = collider.physics().body();
    ph::Physics& physics = *Game::instance->physics;
    // TODO: May want to use MoveKinematic
    physics.interface().SetPosition(body, {position[0], position[1], position[2]}, JPH::EActivation::Activate);
}
