#pragma once

#include "../Entity.h"

#pragma region ForwardDecl
class ParticleEmitter;
#pragma endregion

class CheckpointMarkerEntity : public scene::Entity {
   private:
    inline static const float RADIUS_ = 3.0f;
    inline static const float SPEED_ = 12.0f;
    scene::NodeRef target_ = {};
    ParticleEmitter *emitter1_ = nullptr;
    ParticleEmitter *emitter2_ = nullptr;
    float angle_ = 0;

    void updateEmitter_(ParticleEmitter *emitter, float angle);

   public:
    CheckpointMarkerEntity(scene::SceneRef scene) : scene::Entity(scene) {
    }

    virtual ~CheckpointMarkerEntity();

    void init() override;

    void update(float time_delta) override;

    void setTarget(scene::NodeRef target) {
        target_ = target;
    }
};