#pragma once

#include "../Entity.h"

class MillObstacleEntity : public scene::NodeEntity {
   private:
    scene::NodeRef colliderRef;
    scene::NodeRef meshRef;
    glm::quat initialBladeQuat;
    float cycleTime;
    float bladeAngle = 0.0;

   public:
    MillObstacleEntity(scene::SceneRef scene, scene::NodeRef node) : scene::NodeEntity(scene, node) {
    }

    virtual ~MillObstacleEntity(){};

    void init() override;

    void update(float time_delta) override;

    void prePhysicsUpdate() override;
};