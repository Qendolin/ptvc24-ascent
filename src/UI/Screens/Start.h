#pragma once

#include "../Screen.h"

class StartScreen : public Screen {
   private:
   public:
    StartScreen();
    virtual ~StartScreen();

    void draw() override;
};