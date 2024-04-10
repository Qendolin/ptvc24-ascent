#pragma once

#include <tweeny/tweeny.h>

#include "../../GL/Texture.h"
#include "../Screen.h"

class MainMenuScreen : public Screen {
   private:
    gl::Texture* titleImage = nullptr;
    inline static auto titleOpacity = tweeny::from(0.0).to(0.0).during(300).to(1.0).during(1000).via(tweeny::easing::cubicIn);

   public:
    MainMenuScreen();
    ~MainMenuScreen();

    void draw() override;
};