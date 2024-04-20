#pragma once

#include "../../GL/Declarations.h"
#include "../../Tween.h"
#include "../Screen.h"

class MainMenuScreen : public Screen {
   private:
    gl::Texture* titleImage = nullptr;
    tweeny::tween<float> titleOpacity = tweeny::from(0.0f).to(0.0f).during(100).to(1.0f).during(1000).via(tweeny::easing::cubicIn);

   public:
    MainMenuScreen();
    virtual ~MainMenuScreen();

    void draw() override;
};