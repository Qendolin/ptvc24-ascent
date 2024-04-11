#pragma once

#include <tweeny/tweeny.h>

#include "../Entity.h"

class TestObstacleEntity : public scene::NodeEntity {
   private:
    scene::NodeRef from;
    scene::NodeRef to;
    tweeny::tween<float, float, float> tween;

    scene::NodeRef collider;
    scene::NodeRef mesh;

   public:
    TestObstacleEntity(scene::SceneRef scene, scene::NodeRef node) : scene::NodeEntity(scene, node) {
    }

    void init() override;

    void update() override;

    void prePhysicsUpdate() override;
};