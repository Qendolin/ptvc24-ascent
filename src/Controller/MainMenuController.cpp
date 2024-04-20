#include "MainMenuController.h"

#include "../Game.h"
#include "../Input.h"
#include "../UI/Screens/MainMenu.h"
#include "../UI/Screens/Settings.h"
#include "../Window.h"
#include "MainController.h"

MainMenuController::MainMenuController(Game &game) : AbstractController(game) {}

MainMenuController::~MainMenuController() = default;

void MainMenuController::load() {
    menuScreen = std::make_unique<MainMenuScreen>();
}

void MainMenuController::unload() {
}

void MainMenuController::update() {
    if (menuScreen && menuScreen->isClosed()) {
        switch (menuScreen->action) {
            case MainMenuScreen::Action::Play:
                game.input->captureMouse();
                game.queueController<MainController>();
                break;
            case MainMenuScreen::Action::Settings:
                settingsScreen = std::make_unique<SettingsScreen>(game.settings.get());
                break;
            case MainMenuScreen::Action::Quit:
                glfwSetWindowShouldClose(game.window, true);
                break;
        }

        menuScreen = nullptr;
    }
    if (settingsScreen && settingsScreen->isClosed()) {
        menuScreen = std::make_unique<MainMenuScreen>();
        settingsScreen = nullptr;
    }
}

void MainMenuController::render() {
    if (menuScreen)
        menuScreen->draw();
    if (settingsScreen)
        settingsScreen->draw();
}
