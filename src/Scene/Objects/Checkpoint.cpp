#include "Checkpoint.h"

#include "../../Controller/MainController.h"
#include "../../Debug/Direct.h"
#include "../../Game.h"
#include "../../Physics/Physics.h"
#include "../../Utils.h"

using namespace scene;

void CheckpointEntity::init() {
    sensorRef_ = base.find("*.*.Sensor");

    physics().contactListener->RegisterCallback(sensorRef_.physics().body(), [this](ph::SensorContact contact) {
        if (contact.persistent) return;
        this->onTriggerActivated();
    });

    std::string next_name = base.prop<std::string>("next_checkpoint", "");
    LOG_DEBUG("Next of " + base.name() + " is " + next_name);
    nextCheckpointRef_ = scene.byName(next_name);

    NodeRef respawn_ref = base.find("*.Respawn");
    respawnTransformation_ = respawn_ref.transform();
}

void CheckpointEntity::onTriggerActivated() {
    Trigger trigger = sensorRef_.physics().trigger();

    MainController& controller = dynamic_cast<MainController&>(*game().controller);
    controller.raceManager.onCheckpointEntered(this);
}

void CheckpointEntity::update() {
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