#pragma once

class Game;

class AbstractController {
   protected:
    Game &game;

   public:
    AbstractController(Game &game) : game(game) {
    }

    virtual ~AbstractController() = default;

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

    virtual bool useHdr() {
        return false;
    }
};
