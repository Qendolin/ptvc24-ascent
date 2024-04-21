#include "MainMenuController.h"

#include "../Game.h"
#include "../Input.h"
#include "../UI/Screens/MainMenu.h"
#include "../UI/Screens/Settings.h"
#include "../Window.h"
#include "MainController.h"

MainMenuController::MainMenuController(Game &game)
    : AbstractController(game),
      menuScreen(std::make_unique<MainMenuScreen>()),
      settingsScreen(std::make_unique<SettingsScreen>())  //
{
}

void MainMenuController::load() {
    if (menuScreen->closed())
        menuScreen->open();
}

void MainMenuController::unload() {
}

void MainMenuController::update() {
    if (menuScreen->resetFlag()) {
        switch (menuScreen->action) {
            case MainMenuScreen::Action::Play:
                game.input->captureMouse();
                game.queueController<MainController>();
                break;
            case MainMenuScreen::Action::Settings:
                settingsScreen->open(game.settings.get());
                break;
            case MainMenuScreen::Action::Quit:
                glfwSetWindowShouldClose(game.window, true);
                break;
        }
    }

    if (settingsScreen->resetFlag()) {
        menuScreen->open();
    }
}

void MainMenuController::render() {
    menuScreen->draw();
    settingsScreen->draw();
}
