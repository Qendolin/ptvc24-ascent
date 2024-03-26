#pragma once

#include <tweeny/tweeny.h>

#include "../../GL/Texture.h"
#include "../Screen.h"

class MainMenuScreen : public Screen {
   private:
    GL::Texture* titleImage = nullptr;
    inline static auto titleOpacity = tweeny::from(0.0).to(1.0).during(2000);

   public:
    MainMenuScreen();
    ~MainMenuScreen();

    virtual void draw() override;
};