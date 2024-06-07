#include "Entity.h"

#include "../Game.h"
#include "../Physics/Physics.h"
#include "../Util/Log.h"

namespace scene {

Game& Entity::game() {
    return Game::get();
}

ph::Physics& Entity::physics() {
    return *Game::get().physics;
}

NodeEntity* NodeEntityFactory::create(std::string type, SceneRef scene, NodeRef base) const {
    if (registered_.count(type) == 0)
        PANIC("Entity type '" + type + "' was not registered.");
    return registered_.at(type)(scene, base);
}

}  // namespace scene
