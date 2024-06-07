#pragma once

#include "../Entity.h"
#include "Propeller.h"

class CheckpointEntity : public scene::NodeEntity {
   private:
    scene::NodeRef sensorRef_;
    scene::NodeRef nextCheckpointRef_;
    scene::TransformRef respawnTransformation_;

    Propeller propellerLeft_;
    Propeller propellerRight_;

   public:
    CheckpointEntity(scene::SceneRef scene, scene::NodeRef base) : scene::NodeEntity(scene, base) {
    }

    virtual ~CheckpointEntity(){};

    void init() override;

    void update(float time_delta) override;

    void onTriggerActivated(JPH::BodyID& body);

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
