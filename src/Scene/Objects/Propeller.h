#pragma once

#include "../Entity.h"

class Propeller {
    scene::NodeRef node;
    glm::quat initial;
    glm::quat delta;
    float speed;
    float angle = 0;

   public:
    Propeller() = default;
    Propeller(scene::NodeRef node, float speed);

    void update(float time_delta);
};