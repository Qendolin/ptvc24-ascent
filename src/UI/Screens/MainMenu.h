#pragma once

#include "../../GL/Declarations.h"
#include "../../Tween.h"
#include "../Screen.h"

class MainMenuScreen : public Screen {
   private:
    gl::Texture* titleImage_ = nullptr;
    gl::Texture* backgroundImage_ = nullptr;
    tweeny::tween<float> titleOpacity_ = tweeny::from(0.0f).to(0.0f).during(100).to(1.0f).during(1000).via(tweeny::easing::cubicIn);

    void draw_() override;

   public:
    enum class Action {
        None,
        Play,
        Settings,
        Quit
    };

    Action action = Action::None;

    MainMenuScreen();
    ~MainMenuScreen();

    void open();
};