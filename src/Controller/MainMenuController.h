#pragma once

#include <memory>

#include "AbstractController.h"

class MainMenuScreen;
class SettingsScreen;

class MainMenuController : public AbstractController {
   private:
    std::unique_ptr<MainMenuScreen> menuScreen;
    std::unique_ptr<SettingsScreen> settingsScreen;

   public:
    MainMenuController(Game &game);

    virtual ~MainMenuController();

    void load() override;

    void unload() override;

    void update() override;

    void render() override;
};
