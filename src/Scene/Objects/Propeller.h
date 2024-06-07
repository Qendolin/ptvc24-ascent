#pragma once

#include "../Entity.h"

class Propeller {
    scene::NodeRef blades;
    glm::quat initial;
    float speed;
    float angle = 0;

   public:
    scene::NodeRef node;
    
    Propeller() = default;
    Propeller(scene::NodeRef node, float speed);

    void update(float time_delta);
};