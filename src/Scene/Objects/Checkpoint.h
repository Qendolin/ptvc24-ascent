#pragma once

#include "../Entity.h"

class CheckpointEntity : public Scene::NodeEntity {
   private:
    Scene::NodeRef sensor;

   public:
    CheckpointEntity(Scene::SceneRef scene, Scene::NodeRef base) : Scene::NodeEntity(scene, base) {
    }

    void init() override;

    void update() override;

    void onTriggerActivated();
};
