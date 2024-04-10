#pragma once

#include <tweeny/tweeny.h>

#include "../Entity.h"

class TestObstacleEntity : public Scene::NodeEntity {
   private:
    Scene::NodeRef from;
    Scene::NodeRef to;
    tweeny::tween<float, float, float> tween;

    Scene::NodeRef collider;
    Scene::NodeRef mesh;

   public:
    TestObstacleEntity(Scene::SceneRef scene, Scene::NodeRef node) : Scene::NodeEntity(scene, node) {
    }

    void init() override;

    void update() override;

    void prePhysicsUpdate() override;
};