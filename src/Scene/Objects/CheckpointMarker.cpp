#include "CheckpointMarker.h"

#include "../../Game.h"
#include "../../Particles/ParticleSystem.h"

using namespace scene;

CheckpointMarkerEntity::~CheckpointMarkerEntity() {
    game().particles->remove(emitter1_);
    game().particles->remove(emitter2_);
}

void CheckpointMarkerEntity::init() {
    auto settings = ParticleSettings{
        .frequency = Range<float>(100.0f),
        .count = Range<int>(3),
        .life = Range<float>(0.5f, 0.6f),

        .position = glm::vec3(0, 100, 0),
        .direction = glm::vec3(0, 1, 0),
        .spread = Range<float>(1, 7),
        .gravity = glm::vec3(0, 0, 0),
        .velocity = Range<float>(3, 4),

        .gravityFactor = Range<float>(1.0f),
        .drag = Range<float>(0),
        .rotation = Range<float>(0, 0),
        .revolutions = Range<float>(60, 120),
        .emissivity = 2.0,
        .size = glm::vec2(0.02f, 0.02f),
        .stretching = 3.0f,
    };
    emitter1_ = game().particles->add(settings, "fire");
    emitter2_ = game().particles->add(settings, "fire");
}

void CheckpointMarkerEntity::updateEmitter_(ParticleEmitter* emitter, float angle) {
    TransformRef transform = target_.transform();
    glm::vec3 pos_local = glm::vec3(glm::cos(angle), glm::sin(angle), 0);
    glm::vec3 vel_local = -1.0f * glm::vec3(pos_local.x, pos_local.y, 0);
    pos_local *= RADIUS_;
    emitter->settings().position = transform.matrix() * glm::vec4(pos_local, 1);
    emitter->settings().direction = transform.rotation() * vel_local;
}

void CheckpointMarkerEntity::update(float time_delta) {
    if (target_.isInvalid()) return;

    updateEmitter_(emitter1_, angle_);
    updateEmitter_(emitter2_, angle_ + glm::pi<float>());
    angle_ += SPEED_ * time_delta;
}