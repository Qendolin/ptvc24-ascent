#pragma once

#include "../../ScoreManager.h"
#include "../../Tween.h"
#include "../Screen.h"

class ScoreScreen : public Screen {
   private:
    // clang-format off
    tweeny::tween<float> appearOpacity =
        tweeny::from(0.0f)
            .to(0.0f).during(300)
            .to(1.0f).during(300)
            .to(2.0f).during(300)
            .to(3.0f).during(300)
            .to(4.0f).during(300);
    tweeny::tween<float> graphProgress = 
        tweeny::from(0.0f)
            .to(1.0).during(750).via(tweeny::easing::quadraticIn);
    // clang-format on

    void drawPerformance_();

   public:
    ScoreEntry score;

    ScoreScreen(ScoreEntry score);
    virtual ~ScoreScreen();

    void draw() override;
};