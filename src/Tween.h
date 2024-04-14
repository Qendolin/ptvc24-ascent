#pragma once

#include <tweeny/tweeny.h>

// References:
// https://mobius3.github.io/tweeny/
//
// The "tweeny" tweens need to be updated in discrete time steps.
// This game uses one milliseconds as the smallest unit. (1000 tween units = 1 second)
// The purpose of this class is to aggregate frame times and calculate the `step` value accordingly.
// Examples using the notation: `(carry, step) > time_delta > (next_carry, next_step)`
// 1: `(0.0ms, 0) > 0.4ms > (0.4ms, 0) > 0.4ms > (0.8ms, 0) > 0.4ms > (0.2ms, 1) > 0.4ms > (0.6ms, 0)`
// 2: `(0.0ms, 0) > 12.5ms > (0.5ms, 12) > 16.0ms > (0.5ms, 16) > 10.5ms > (0.0ms, 10)`
class TweenSystem {
   private:
    // the carry aggregates sub-millisecond times until a full millisecond is reached.
    float carry_ = 0.0;
    // the step stores the amount of tweeny time units that each tween should be advanced for this frame.
    int step_ = 0;

   public:
    TweenSystem();
    ~TweenSystem();

    void update(float time_delta);

    // Takes a tween and advances it
    template <typename T, typename... Ts>
    void step(tweeny::tween<T, Ts...> &tween) const {
        tween.step(step_);
    }
};