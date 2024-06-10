#include "Propeller.h"

#include <glm/gtx/fast_trigonometry.hpp>

Propeller::Propeller(scene::NodeRef node, float speed) {
    this->node = node;
    this->blades = node.find("*.Blades.*");
    this->blades.transform().setParent(node.transform());
    this->speed = speed;
    this->initial = blades.transform().rotation();
}

void Propeller::update(float time_delta) {
    angle += speed * time_delta * glm::pi<float>();
    angle = glm::wrapAngle(angle);

    glm::quat rotation_local = glm::angleAxis(angle, glm::vec3(0, 1, 0));
    glm::quat rotation_world = initial * rotation_local;

    blades.transform().setRotation(rotation_world);
    blades.graphics().setTransformFromNode();
    node.graphics().setTransformFromNode();
}