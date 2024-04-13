#pragma once

#include <memory>

#include "GameController.h"

class Screen;

class MainMenuController : public GameController {
   private:
    std::unique_ptr<Screen> screen;

   public:
    MainMenuController(Game &game);

    virtual ~MainMenuController();

    void load() override;

    void unload() override;

    void update() override;

    void render() override;
};
