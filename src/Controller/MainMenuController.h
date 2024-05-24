#pragma once

#include <memory>

#include "AbstractController.h"

class MainMenuScreen;
class SettingsScreen;
class Music;

class MainMenuController : public AbstractController {
   private:
    const std::unique_ptr<MainMenuScreen> menuScreen;
    const std::unique_ptr<SettingsScreen> settingsScreen;
    std::unique_ptr<Music> menuMusic;

   public:
    MainMenuController(Game &game);

    virtual ~MainMenuController() = default;

    void load() override;

    void unload() override;

    void update() override;

    void render() override;
};
