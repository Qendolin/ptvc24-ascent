#pragma once

#include "../../ScoreManager.h"
#include "../../Tween.h"
#include "../Screen.h"

class ScoreScreen : public Screen {
   private:
    ScoreEntry score_ = {};

    // clang-format off
    tweeny::tween<float> appearOpacity_ =
        tweeny::from(0.0f)
            .to(0.0f).during(300)
            .to(1.0f).during(300)
            .to(2.0f).during(300)
            .to(3.0f).during(300)
            .to(4.0f).during(300);
    tweeny::tween<float> graphProgress_ = 
        tweeny::from(0.0f)
            .to(1.0).during(750).via(tweeny::easing::quadraticIn);
    // clang-format on

    void drawPerformance_();

    void draw_() override;

   public:
    ScoreScreen() = default;
    virtual ~ScoreScreen() = default;

    void open(ScoreEntry score);
};