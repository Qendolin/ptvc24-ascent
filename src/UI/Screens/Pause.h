#pragma once

#include "../Screen.h"

class PauseScreen : public Screen {
   private:
   public:
    enum class Action {
        None,
        Respawn,
        Exit
    };

    Action action = Action::None;

    PauseScreen();
    virtual ~PauseScreen();

    void draw() override;

    void close() override;
};