#pragma once

#include "../../Tween.h"
#include "../Screen.h"

class FadeOverlay : public Screen {
   private:
    float from_;
    float to_;
    float duration_;
    double startTime_;

   public:
    FadeOverlay();
    virtual ~FadeOverlay();

    void fade(float from, float to, float duration);

    void draw() override;
};