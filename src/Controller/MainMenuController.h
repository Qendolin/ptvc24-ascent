#pragma once

#include <memory>

#include "AbstractController.h"

class Screen;

class MainMenuController : public AbstractController {
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
