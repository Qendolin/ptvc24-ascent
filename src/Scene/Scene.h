#pragma once

#include <jolt/Jolt.h>
#include <jolt/Physics/Body/BodyID.h>

#include <any>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <map>
#include <string>
#include <vector>

#pragma region ForwardDecl
namespace loader {
class SceneData;
struct Node;
struct InstanceAttributes;
}  // namespace loader
#pragma endregion

namespace scene {

using GraphicsInstanceAttributes = loader::InstanceAttributes;

class Properties {
   private:
    std::map<std::string, std::any> map_ = {};

    void checkKeyExists_(const std::string& key) const;

   public:
    Properties() {}
    Properties(const std::map<std::string, std::any>& map) : map_(map) {}

    template <typename T>
    T get(const std::string& key) const {
        checkKeyExists_(key);
        return std::any_cast<T>(map_.at(key));
    }

    template <typename T>
    T get(const std::string& key, T default_value) const {
        if (map_.count(key) == 0) return default_value;
        return std::any_cast<T>(map_.at(key));
    }
};

class Tags {
   private:
    std::vector<std::string> tags_ = {};

   public:
    Tags() {}
    Tags(const std::vector<std::string>& tags) : tags_(tags) {}

    bool has(const std::string& tag) const {
        return std::find(tags_.cbegin(), tags_.cend(), tag) != tags_.cend();
    }
};

struct Node {
    std::string name;
    int32_t index;

    int32_t graphics;
    int32_t physics;
    int32_t entity;

    int32_t parent;
    std::vector<int32_t> children;

    Properties properties;
    Tags tags;
};

struct Graphics {
    GraphicsInstanceAttributes* attributes;
};

struct Trigger {
    bool enabled = false;
    std::string action = "";
    std::string argument = "";
};

struct Physics {
    JPH::BodyID body;
    Trigger trigger;
};

struct Transform {
    glm::vec3 t = glm::vec3(0.0);
    glm::quat r = glm::quat();
    glm::vec3 s = glm::vec3(1.0);

    glm::mat4 matrix() {
        glm::mat4 scale = glm::scale(glm::mat4(1.0), s);
        glm::mat4 rotation = glm::mat4_cast(r);
        glm::mat4 translation = glm::translate(glm::mat4(1.0), t);
        return translation * rotation * scale;
    }
};

// forward declaration
class NodeEntityFactory;
class Entity;

class Scene {
   private:
    int32_t convertNodes_(const loader::SceneData& scene, const NodeEntityFactory& factory, const loader::Node& node, int32_t parent);
    bool initialized_ = false;
    std::vector<Entity*> initializationQueue_;

   public:
    std::vector<Node> nodes;
    // map node names to indices
    std::unordered_map<std::string, int32_t> nodesByName;
    // map body ids to indices, assues that body ids don't get reused
    std::unordered_map<JPH::BodyID, int32_t> nodesByBodyID;
    std::vector<Graphics> graphics;
    std::vector<Physics> physics;
    std::vector<Transform> transforms;
    std::vector<Entity*> entities;

    Scene(const loader::SceneData& scene, NodeEntityFactory& factory);

    ~Scene();

    Node& createPhysicsNode(std::string name, const scene::Physics& physics);

    void callEntityInit();

    bool initialized() {
        return initialized_;
    }

    void callEntityUpdate(float time_delta);

    void callEntityPrePhysicsUpdate();

    void callEntityPostPhysicsUpdate();
};

class TransformRef {
   private:
    Scene* scene_ = nullptr;
    int32_t index_ = -1;

   public:
    TransformRef() : scene_(nullptr), index_(-1) {
    }
    TransformRef(Scene& scene, int32_t index)
        : scene_(&scene), index_(index) {
    }

    bool isValid() const {
        return scene_ != nullptr && index_ >= 0;
    }

    bool isInvalid() const {
        return !isValid();
    }

    operator bool() const {
        return isValid();
    }

    glm::vec3 position() {
        return scene_->transforms[index_].t;
    }

    void setPosition(const glm::vec3 position) {
        scene_->transforms[index_].t = position;
    }

    void setPosition(float x, float y, float z) {
        scene_->transforms[index_].t.x = x;
        scene_->transforms[index_].t.y = y;
        scene_->transforms[index_].t.z = z;
    }

    glm::vec3 scale() {
        return scene_->transforms[index_].s;
    }

    void setScale(const glm::vec3 scale) {
        scene_->transforms[index_].s = scale;
    }

    glm::quat rotation() {
        return scene_->transforms[index_].r;
    }

    void setRotation(const glm::quat rotation) {
        scene_->transforms[index_].r = rotation;
    }

    glm::mat4 matrix() {
        return scene_->transforms[index_].matrix();
    }
};

class GraphicsRef {
   private:
    Scene* scene_ = nullptr;
    int32_t index_ = -1;
    int32_t node_ = -1;

   public:
    GraphicsRef() : scene_(nullptr), index_(-1), node_(-1) {
    }

    GraphicsRef(Scene& scene, int32_t index, int32_t node)
        : scene_(&scene), index_(index), node_(node) {
    }

    bool isValid() const {
        return scene_ != nullptr && index_ >= 0;
    }

    bool isInvalid() const {
        return !isValid();
    }

    operator bool() const {
        return isValid();
    }

    void setTransformFromNode();

    void setTransform(const glm::mat4& matrix);
};

class PhysicsRef {
   private:
    Scene* scene_ = nullptr;
    int32_t index_ = -1;
    int32_t node_ = -1;

   public:
    PhysicsRef() : scene_(nullptr), index_(-1), node_(-1) {
    }
    PhysicsRef(Scene& scene, int32_t index, int32_t node)
        : scene_(&scene), index_(index), node_(node) {
    }

    const JPH::BodyID body() {
        return scene_->physics[index_].body;
    };

    bool hasTrigger() {
        return scene_->physics[index_].trigger.enabled;
    }

    Trigger trigger() {
        return scene_->physics[index_].trigger;
    }

    bool isValid() const {
        return scene_ != nullptr && index_ >= 0;
    }

    bool isInvalid() const {
        return !isValid();
    }

    operator bool() const {
        return isValid();
    }
};

class NodeRef {
   private:
    Scene* scene_ = nullptr;
    int32_t index_ = -1;

   public:
    NodeRef() : scene_(nullptr), index_(-1) {
    }

    NodeRef(Scene& scene, int32_t index)
        : scene_(&scene), index_(index) {
    }

    std::string name() const {
        return scene_->nodes[index_].name;
    }

    bool isValid() const {
        return scene_ != nullptr && index_ >= 0;
    }

    bool isInvalid() const {
        return !isValid();
    }

    operator bool() const {
        return isValid();
    }

    bool isRoot() const {
        return index_ == 0;
    }

    TransformRef transform() const {
        return TransformRef(*scene_, index_);
    }

    bool hasGraphics() const {
        return scene_->nodes[index_].graphics >= 0;
    }

    GraphicsRef graphics() const {
        return GraphicsRef(*scene_, scene_->nodes[index_].graphics, index_);
    }

    bool hasPhysics() const {
        return scene_->nodes[index_].physics >= 0;
    }

    PhysicsRef physics() const {
        return PhysicsRef(*scene_, scene_->nodes[index_].physics, index_);
    }

    bool hasEntity() const {
        return scene_->nodes[index_].entity >= 0;
    }

    template <class T = Entity>
    T* entity() const {
        Entity* entity = scene_->entities[scene_->nodes[index_].entity];
        return dynamic_cast<T*>(entity);
    }

    std::vector<NodeRef> children() const {
        std::vector<NodeRef> result;
        std::vector<int32_t>& children = scene_->nodes[index_].children;
        result.reserve(children.size());
        for (int32_t index : scene_->nodes[index_].children) {
            result.emplace_back(*scene_, index);
        }
        return result;
    }

    NodeRef parent() const {
        return NodeRef(*scene_, scene_->nodes[index_].parent);
    }

    NodeRef find(const std::string& path) const;

    NodeRef find(const std::string& path, std::function<bool(NodeRef& node)> predicate) const;

    bool hasTag(const std::string& tag) const {
        return scene_->nodes[index_].tags.has(tag);
    }

    template <typename T>
    T prop(const std::string& key) const {
        return scene_->nodes[index_].properties.get<T>(key);
    }

    template <typename T>
    T prop(const std::string& key, T default_value) const {
        return scene_->nodes[index_].properties.get<T>(key, default_value);
    }
};

class SceneRef {
   private:
    Scene* scene_ = nullptr;

    /**
     * Removes the number from the end of the name. E.g.: `name.002` becomes `name`
     */
    std::string removeTrailingNumberFromName_(std::string name) const;

    /**
     * @param pattern e.g.: `mesh.*.part`, `*.sensor`
     * @param name a node name without the trailing number
     */
    bool matchPatternAgainstName_(std::string pattern, std::string name) const;

   public:
    SceneRef() : scene_(nullptr) {}

    SceneRef(Scene& scene) : scene_(&scene) {}

    NodeRef find(NodeRef parent, std::string path) const;

    NodeRef find(NodeRef parent, std::function<bool(NodeRef& node)> predicate) const;

    NodeRef byName(std::string name) const;

    NodeRef byPhysicsBody(JPH::BodyID body_id) const {
        if (scene_->nodesByBodyID.count(body_id) == 0) return NodeRef();
        return NodeRef(*scene_, scene_->nodesByBodyID.at(body_id));
    }

    NodeRef root() const {
        return NodeRef(*scene_, 0);
    }

    template <typename T, typename... Args>
    T* create(Args&&... args) {
        T* entity = new T(*this, std::forward<Args>(args)...);
        scene_->entities.push_back(entity);
        if (scene_->initialized()) {
            entity->init();
        }
        return entity;
    }

    bool isValid() const {
        return scene_ != nullptr;
    }

    bool isInvalid() const {
        return !isValid();
    }

    operator bool() const {
        return isValid();
    }
};

}  // namespace scene