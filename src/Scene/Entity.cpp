#include "Entity.h"

#include "../Game.h"

namespace Scene {

Game& Entity::game() {
    return *Game::instance;
}

PH::Physics& Entity::physics() {
    return *Game::instance->physics;
}

}  // namespace Scene
