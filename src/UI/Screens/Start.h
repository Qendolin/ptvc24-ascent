#pragma once

#include "../../Loader/Atlas.h"
#include "../../Tween.h"
#include "../Screen.h"

class StartScreen : public Screen {
   private:
    std::unique_ptr<loader::Atlas> inputPromptsAtlas_;
    tweeny::tween<float> startTextOpacity_ = tweeny::from(0.0f).to(0.0f).during(2000).to(1.0f).during(1500).via(tweeny::easing::cubicIn);

    void draw_() override;

    struct nk_image inputSprite_(std::string key);

   public:
    StartScreen();
    virtual ~StartScreen() = default;

    void open();
};