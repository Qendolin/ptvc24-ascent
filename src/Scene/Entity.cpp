#include "Entity.h"

#include "../Game.h"
#include "../Physics/Physics.h"

namespace scene {

Game& Entity::game() {
    return Game::get();
}

ph::Physics& Entity::physics() {
    return *Game::get().physics;
}

}  // namespace scene
