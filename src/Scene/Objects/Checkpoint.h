#pragma once

#include "../Entity.h"

class CheckpointEntity : public scene::NodeEntity {
   private:
    scene::NodeRef sensor;

   public:
    CheckpointEntity(scene::SceneRef scene, scene::NodeRef base) : scene::NodeEntity(scene, base) {
    }

    void init() override;

    void update() override;

    void onTriggerActivated();
};
