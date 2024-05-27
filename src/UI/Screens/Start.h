#pragma once

#include "../../Loader/Atlas.h"
#include "../Screen.h"

class StartScreen : public Screen {
   private:
    std::unique_ptr<loader::Atlas> inputPromptsAtlas_;

    void draw_() override;

    struct nk_image inputSprite_(std::string key);

   public:
    StartScreen();
    virtual ~StartScreen() = default;

    void open();
};