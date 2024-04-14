#include "MainMenuController.h"

#include "../Game.h"
#include "../UI/Screens/MainMenu.h"
#include "MainController.h"

MainMenuController::MainMenuController(Game &game) : GameController(game) {}

MainMenuController::~MainMenuController() = default;

void MainMenuController::load() {
    screen = std::make_unique<MainMenuScreen>();
}

void MainMenuController::unload() {
}

void MainMenuController::update() {
    if (screen->isClosed()) {
        game.queueController<MainController>();
    }
}

void MainMenuController::render() {
    screen->draw();
}
