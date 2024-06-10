#pragma once

#include "../Entity.h"

#pragma region ForwardDecl
class ParticleEmitter;
#pragma endregion

class GoalFireworkEntity : public scene::NodeEntity {
   private:
    scene::NodeRef sensorRef_;
    scene::NodeRef jetLeftRef_;
    scene::NodeRef jetRightRef_;
    bool activated_ = false;
    ParticleEmitter *emitterLeft_ = nullptr;
    ParticleEmitter *emitterRight_ = nullptr;

   public:
    GoalFireworkEntity(scene::SceneRef scene, scene::NodeRef node) : scene::NodeEntity(scene, node) {
    }

    virtual ~GoalFireworkEntity();

    void init() override;

    void update(float time_delta) override {}

    void onTriggerActivated(JPH::BodyID &body);
};
