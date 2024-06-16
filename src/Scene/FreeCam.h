#pragma once

#include "../Util/Timer.h"
#include "Entity.h"

#pragma region ForwardDecl
class Camera;
#pragma endregion

class InputSmoother {
    float smoothedSum = 0;
    float actualSum = 0;
    float latency = 0;

   public:
    float smooth(float original, float smoother) {
        actualSum += original;
        float d = actualSum - smoothedSum;
        float e = latency + 0.5f * (d - latency);
        float sign = glm::sign(d);
        if (sign * d > sign * latency) {
            d = e;
        }
        latency = e;
        smoothedSum += d * smoother;
        return d * smoother;
    }
};

// allows free camera movement for debugging
class FreeCamEntity {
   private:
    // Flying speed in m/s
    inline static const float BASE_SPEED = 10.0f;
    inline static const float MIN_SPEED = 0.1f;
    inline static const float MAX_SPEED = 1000.0f;
    // Change speed by scrolling
    float speed_ = BASE_SPEED;

    InputSmoother xSmooth;
    InputSmoother ySmooth;

   public:
    Camera& camera;

    FreeCamEntity(Camera& camera) : camera(camera) {
    }

    virtual ~FreeCamEntity() = default;

    void update(float time_delta);
};