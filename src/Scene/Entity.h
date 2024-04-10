#pragma once

#include "Scene.h"

// forward declarations
class Game;
namespace ph {
class Physics;
}

namespace scene {

// Entity represents objects that need to process game logic
class Entity {
   protected:
    Game& game();

    ph::Physics& physics();

   public:
    virtual ~Entity() {}

    // Called at scene creation
    virtual void init() = 0;

    // Called every frame, not before physics update
    virtual void update() = 0;

    // Called just before every physics step
    virtual void prePhysicsUpdate() {}

    // Called just after every physics step
    virtual void postPhysicsUpdate() {}
};

class NodeEntity : public Entity {
   protected:
    SceneRef scene;
    NodeRef base;

   public:
    virtual ~NodeEntity() {}

    NodeEntity(SceneRef scene, NodeRef base) : scene(scene), base(base) {
    }
};

class NodeEntityFactory {
   private:
    std::map<std::string, std::function<NodeEntity*(SceneRef, NodeRef)>> registered_;

   public:
    NodeEntity* create(std::string type, SceneRef scene, NodeRef base) const {
        return registered_.at(type)(scene, base);
    }

    template <typename T>
    void registerEntity(std::string type) {
        registered_[type] = [](SceneRef scene, NodeRef node) {
            return new T(scene, node);
        };
    }
};

}  // namespace scene
