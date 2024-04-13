#pragma once

class Game;

class GameController {
   protected:
    Game &game;

   public:
    GameController(Game &game) : game(game) {
    }

    virtual ~GameController() = default;

    /**
     * Called when the controller is activated and when assets are reloaded
    */
    virtual void load() = 0;

    /**
     * Called when the cotroller is deactivated and when assets are reloaded
    */
    virtual void unload() = 0;

    /**
     * Called every frame before `render`
    */
    virtual void update() = 0;

    /**
     * Called every frame after `update`
    */
    virtual void render() = 0;
};
