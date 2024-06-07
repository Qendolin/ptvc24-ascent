#pragma once

#include "../../Util/Timer.h"
#include "../Entity.h"

class BoostRingEntity : public scene::NodeEntity {
   private:
    scene::NodeRef sensorRef;
    scene::NodeRef meshRef;
    glm::quat initialRotation;
    Timer cooldown = {};
    float angle = 0;

   public:
    BoostRingEntity(scene::SceneRef scene, scene::NodeRef node) : scene::NodeEntity(scene, node) {
    }

    virtual ~BoostRingEntity(){};

    void init() override;

    void update(float time_delta) override;

    void onTriggerActivated(JPH::BodyID &body);
};