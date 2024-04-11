#include "Entity.h"

#include "../Game.h"

namespace scene {

Game& Entity::game() {
    return *Game::instance;
}

ph::Physics& Entity::physics() {
    return *Game::instance->physics;
}

}  // namespace scene
