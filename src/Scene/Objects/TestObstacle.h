#pragma once

#include <tweeny/tweeny.h>

#include "../Entity.h"

class TestObstacleEntity : public scene::NodeEntity {
   private:
    scene::NodeRef fromRef;
    scene::NodeRef toRef;
    tweeny::tween<float, float, float> tween;

    scene::NodeRef colliderRef;
    scene::NodeRef meshRef;

   public:
    TestObstacleEntity(scene::SceneRef scene, scene::NodeRef node) : scene::NodeEntity(scene, node) {
    }

    virtual ~TestObstacleEntity(){};

    void init() override;

    void update(float time_delta) override;

    void debugDraw() override;

    void prePhysicsUpdate() override;
};