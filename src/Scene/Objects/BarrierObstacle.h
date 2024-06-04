#pragma once

#include <tweeny/tweeny.h>

#include "../Entity.h"

#include "Propeller.h"

class BarrierObstacleEntity : public scene::NodeEntity {
   private:
    scene::NodeRef fromRef;
    scene::NodeRef toRef;
    tweeny::tween<float, float, float> tween;

    scene::NodeRef colliderRef;
    scene::NodeRef meshRef;

    Propeller propellerLeft_;
    Propeller propellerRight_;

   public:
    BarrierObstacleEntity(scene::SceneRef scene, scene::NodeRef node) : scene::NodeEntity(scene, node) {
    }

    virtual ~BarrierObstacleEntity(){};

    void init() override;

    void update(float time_delta) override;

    void debugDraw() override;

    void prePhysicsUpdate() override;
};