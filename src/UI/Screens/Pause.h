#pragma once

#include "../Screen.h"

class PauseScreen : public Screen {
   private:
   public:
    PauseScreen();
    virtual ~PauseScreen();

    void draw() override;

    void close() override;
};