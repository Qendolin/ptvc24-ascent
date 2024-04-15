#pragma once

#include "../Entity.h"

class CheckpointEntity : public scene::NodeEntity {
   private:
    scene::NodeRef sensorRef_;
    scene::NodeRef nextCheckpointRef_;
    scene::TransformRef respawnTransformation_;

   public:
    CheckpointEntity(scene::SceneRef scene, scene::NodeRef base) : scene::NodeEntity(scene, base) {
    }

    virtual ~CheckpointEntity(){};

    void init() override;

    void update() override;

    void onTriggerActivated();

    void debugDraw() override;

    bool hasNextCheckpoint() const {
        return nextCheckpointRef_.isValid() && nextCheckpointRef_.hasEntity();
    }

    CheckpointEntity* nextCheckpoint() const {
        return nextCheckpointRef_.entity<CheckpointEntity>();
    }

    scene::TransformRef respawnTransformation() const {
        return respawnTransformation_;
    }
};
