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
    SceneRef scene;

    Game& game();

    ph::Physics& physics();

   public:
    Entity(SceneRef scene) : scene(scene) {
    }

    virtual ~Entity(){};

    // Called at scene creation
    virtual void init() = 0;

    // Called every frame, not before physics update
    virtual void update() = 0;

    virtual void debugDraw(){};

    // Called just before every physics step
    virtual void prePhysicsUpdate() {}

    // Called just after every physics step
    virtual void postPhysicsUpdate() {}
};

// An entity that has an associated node and is automatically created during loding
class NodeEntity : public Entity {
   protected:
    NodeRef base;

   public:
    virtual ~NodeEntity() {}

    NodeEntity(SceneRef scene, NodeRef base) : Entity(scene), base(base) {
    }
};

// A factory for that creates new NodeEntities for the given nodes
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
