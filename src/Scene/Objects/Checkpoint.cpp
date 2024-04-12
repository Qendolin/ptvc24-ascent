#include "Checkpoint.h"

#include "../../Game.h"
#include "../../Physics/Physics.h"
#include "../../Utils.h"

using namespace scene;

void CheckpointEntity::init() {
    sensor = base.find("*.*.Sensor");

    physics().contactListener->RegisterCallback(sensor.physics().body(), [this](ph::SensorContact contact) {
        if (contact.persistent) return;
        this->onTriggerActivated();
    });

    std::string next = base.prop<std::string>("next_checkpoint", "");
    LOG_DEBUG("Next of " + base.name() + " is " + next);
}

void CheckpointEntity::onTriggerActivated() {
    Trigger trigger = sensor.physics().trigger();
    LOG_DEBUG("Triggered action " + trigger.action + "(" + trigger.argument + ") on checkpoint '" + base.name() + "'");
}

void CheckpointEntity::update() {
}