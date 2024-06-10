#include "GoalFirework.h"

#include "../../Controller/MainController.h"
#include "../../Debug/Direct.h"
#include "../../Game.h"
#include "../../Particles/ParticleSystem.h"
#include "../../Physics/Physics.h"
#include "../../Util/Log.h"

using namespace scene;

GoalFireworkEntity::~GoalFireworkEntity() {
    if (emitterLeft_)
        game().particles->remove(emitterLeft_);
    if (emitterRight_)
        game().particles->remove(emitterRight_);
}

void GoalFireworkEntity::init() {
    sensorRef_ = base.find("*.*.Sensor");

    physics().contactListener->RegisterCallback(sensorRef_.physics().body(), [this](ph::SensorContact contact) {
        if (contact.persistent) return;
        this->onTriggerActivated(contact.other);
    });

    jetLeftRef_ = base.find("Jet.Left");
    jetRightRef_ = base.find("Jet.Right");
}

void GoalFireworkEntity::onTriggerActivated(JPH::BodyID& body) {
    if (activated_) return;
    NodeRef contactNode = scene.byPhysicsBody(body);
    if (contactNode.isInvalid() || !contactNode.hasTag("player"))
        return;
    activated_ = true;
    auto settings = ParticleSettings{
        .frequency = Range<float>(60.0f),
        .count = Range<int>(2),
        .life = Range<float>(1.4f, 1.7f),

        .position = glm::vec3(0, 100, 0),
        .direction = glm::vec3(0, 1, 0),
        .spread = Range<float>(1, 7),
        .gravity = glm::vec3(0, -9.81, 0),
        .velocity = Range<float>(15, 17),

        .gravityFactor = Range<float>(1.4f),
        .drag = Range<float>(0),
        .rotation = Range<float>(0, 0),
        .revolutions = Range<float>(0),
        .emissivity = 1.2f,
        .size = glm::vec2(0.02f, 0.02f),
        .stretching = 3.0f,
    };
    settings.position = jetLeftRef_.transform().position();
    settings.direction = jetLeftRef_.transform().rotation() * glm::vec3(0, 1, 0);
    emitterLeft_ = game().particles->add(settings, "fire");

    settings.direction = jetRightRef_.transform().rotation() * glm::vec3(0, 1, 0);
    settings.position = jetRightRef_.transform().position();
    emitterRight_ = game().particles->add(settings, "fire");
}