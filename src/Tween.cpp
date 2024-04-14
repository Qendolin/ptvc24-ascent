#include "Tween.h"

TweenSystem::TweenSystem() = default;
TweenSystem::~TweenSystem() = default;

void TweenSystem::update(float time_delta) {
    carry_ += time_delta * 1000.0f;
    step_ = (int)floor(carry_);
    carry_ -= step_;
}