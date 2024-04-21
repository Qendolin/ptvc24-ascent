#pragma once

#include "../Screen.h"

class StartScreen : public Screen {
    private:
    void draw_() override;

   public:
    StartScreen() = default;
    virtual ~StartScreen() = default;

    void open();
};