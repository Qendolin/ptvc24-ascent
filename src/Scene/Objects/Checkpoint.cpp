#include "Checkpoint.h"

#include "../../Game.h"

using namespace Scene;

void CheckpointEntity::init() {
    sensor = base.find("*.*.Sensor");

    physics().contactListener->RegisterCallback(sensor.physics().body(), [this](PH::SensorContact contact) {
        if (contact.persistent) return;
        this->onTriggerActivated();
    });

    std::string next = base.prop<std::string>("next_checkpoint", "");
    LOG("Next of " + base.name() + " is " + next);
}

void CheckpointEntity::onTriggerActivated() {
    Trigger trigger = sensor.physics().trigger();
    LOG("Triggered action " + trigger.action + "(" + trigger.argument + ") on checkpoint '" + base.name() + "'");
}

void CheckpointEntity::update() {
}