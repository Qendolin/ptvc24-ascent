#pragma once

#include "../Entity.h"

class CheckpointEntity : public scene::NodeEntity {
   private:
    class Propeller {
        scene::NodeRef node;
        glm::quat initial;
        glm::quat delta;
        float speed;
        float angle = 0;

       public:
        Propeller() = default;
        Propeller(scene::NodeRef node, float speed);

        void update(float time_delta);
    };

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
