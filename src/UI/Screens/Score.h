#pragma once

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
    // clang-format on

   public:
    struct Score {
        float time;
        float penalty;
    };

    Score score;

    ScoreScreen(Score score);
    virtual ~ScoreScreen();

    void draw() override;
};