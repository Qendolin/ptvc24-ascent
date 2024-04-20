#include "Checkpoint.h"

#include <glm/gtx/fast_trigonometry.hpp>

#include "../../Controller/MainController.h"
#include "../../Debug/Direct.h"
#include "../../Game.h"
#include "../../Physics/Physics.h"
#include "../../Util/Log.h"

using namespace scene;

void CheckpointEntity::init() {
    sensorRef_ = base.find("*.*.Sensor");

    physics().contactListener->RegisterCallback(sensorRef_.physics().body(), [this](ph::SensorContact contact) {
        if (contact.persistent) return;
        this->onTriggerActivated();
    });

    std::string next_name = base.prop<std::string>("next_checkpoint", "");
    nextCheckpointRef_ = scene.byName(next_name);

    NodeRef respawn_ref = base.find("*.Respawn");
    respawnTransformation_ = respawn_ref.transform();

    propellerLeft_ = Propeller(base.find("Propeller.Left/*.Blades.*"), -3);
    propellerRight_ = Propeller(base.find("Propeller.Right/*.Blades.*"), 3);
}

void CheckpointEntity::onTriggerActivated() {
    // FIXME: doesn't check if triggered by player
    Trigger trigger = sensorRef_.physics().trigger();

    MainController& controller = dynamic_cast<MainController&>(*game().controller);
    controller.raceManager.onCheckpointEntered(this);
}

CheckpointEntity::Propeller::Propeller(scene::NodeRef node, float speed) {
    this->node = node;
    this->speed = speed;
    initial = node.transform().rotation();
}

void CheckpointEntity::Propeller::update(float time_delta) {
    angle += speed * time_delta * glm::pi<float>();
    angle = glm::wrapAngle(angle);

    glm::quat rotation_local = glm::angleAxis(angle, glm::vec3(0, 1, 0));
    glm::quat rotation_world = initial * rotation_local;

    node.transform().setRotation(rotation_world);
    node.graphics().setTransformFromNode();
}

void CheckpointEntity::rotatePropeller_(NodeRef& node, float angle) {
}

void CheckpointEntity::update(float time_delta) {
    propellerLeft_.update(time_delta);
    propellerRight_.update(time_delta);
}

void CheckpointEntity::debugDraw() {
    DirectBuffer& dd = *Game::get().directDraw;
    scene::TransformRef transform = respawnTransformation();
    dd.unshaded();
    dd.stroke(0.05f);
    dd.axes(transform.matrix(), 2.0);

    if (nextCheckpointRef_.isValid()) {
        dd.color(0, 0, 0);
        dd.line(base.transform().position() + glm::vec3{0, 3, 0}, nextCheckpointRef_.transform().position());
    }
}