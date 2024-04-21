#pragma once

#include "../Screen.h"

class PauseScreen : public Screen {
   private:
    void draw_() override;

   public:
    enum class Action {
        None,
        Respawn,
        Exit
    };

    Action action = Action::None;

    PauseScreen() = default;
    virtual ~PauseScreen() = default;

    void open();

    void close() override;
};