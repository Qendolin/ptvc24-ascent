#pragma once

#include "../../Tween.h"
#include "../Screen.h"

class FadeOverlay : public Screen {
   private:
    float from_ = 0;
    float to_ = 0;
    float duration_ = 1;
    double startTime_ = 0;

    void draw_() override;
   
   public:
    FadeOverlay() {
        opened_ = true;
    };
    virtual ~FadeOverlay() = default;

    void fade(float from, float to, float duration);

};