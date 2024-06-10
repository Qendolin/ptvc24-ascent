#include "BoostRing.h"

#include <glm/gtx/fast_trigonometry.hpp>

#include "../../Audio/Assets.h"
#include "../../Controller/MainController.h"
#include "../../Game.h"
#include "../../Physics/Physics.h"
#include "../Character.h"

using namespace scene;

void BoostRingEntity::init() {
    meshRef = base.find("BoostRing.Mesh");
    initialRotation = meshRef.transform().rotation();
    sensorRef = base.find("Phys.Cylinder.Sensor");
    physics().contactListener->RegisterCallback(sensorRef.physics().body(), [this](ph::SensorContact contact) {
        if (contact.persistent) return;
        this->onTriggerActivated(contact.other);
    });
}

void BoostRingEntity::update(float time_delta) {
    angle += 0.125f * time_delta * glm::pi<float>();
    angle = glm::wrapAngle(angle);

    glm::quat rotation_local = glm::angleAxis(angle, glm::vec3(0, 0, 1));
    glm::quat rotation_world = initialRotation * rotation_local;

    meshRef.transform().setRotation(rotation_world);
    meshRef.graphics().setTransformFromNode();

    cooldown.update(time_delta);
}

void BoostRingEntity::onTriggerActivated(JPH::BodyID& body) {
    NodeRef contactNode = scene.byPhysicsBody(body);
    if (contactNode.isInvalid() || !contactNode.hasTag("player"))
        return;

    if (!cooldown.isZero()) return;

    auto character = contactNode.entity<CharacterEntity>();
    if (!character) return;
    glm::vec3 velocity = character->velocity();
    velocity += base.prop<float>("boost") * (meshRef.transform().rotation() * glm::vec3(0, 0, -1));
    character->setVelocity(velocity);
    game().audio->assets->woosh2.play2dEvent(1.0);

    cooldown = 2.0;
}