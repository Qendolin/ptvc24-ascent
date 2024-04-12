#include "Scene.h"

#include "../Loader/Gltf.h"
#include "../Physics/Physics.h"
#include "../Utils.h"
#include "Entity.h"

namespace scene {

using GraphicsInstanceAttributes = loader::InstanceAttributes;

void Properties::checkKeyExists_(const std::string& key) const {
    if (map_.count(key) == 0)
        PANIC("Property " + key + " does not exist");
}

Scene::Scene(const loader::Scene& scene, NodeEntityFactory& factory) {
    transforms.reserve(scene.count());
    nodes.reserve(scene.count());
    // this will allocate more than needed
    graphics.reserve(scene.count());
    physics.reserve(scene.count());
    convertNodes_(scene, factory, scene.root(), -1);
}

Scene::~Scene() {
    for (auto&& e : entities) {
        delete e;
    }
}

int32_t Scene::convertNodes_(const loader::Scene& scene, const NodeEntityFactory& factory, const loader::Node& node, int32_t parent) {
    int32_t index = nodes.size();
    Node& result = nodes.emplace_back();
    result = {
        .name = node.name,
        .index = index,
        .parent = parent,
        .children = {},
        .properties = Properties(node.properties),
        .tags = node.tags,
    };

    transforms.emplace_back() = {
        .t = node.initialPosition,
        .r = node.initialOrientation,
        .s = node.initialScale,
    };

    // entity
    if (!node.entityClass.empty()) {
        entities.push_back(factory.create(node.entityClass, SceneRef(*this), NodeRef(*this, index)));
    }

    // graphics
    if (node.graphics < 0) {
        result.graphics = -1;
    } else {
        result.graphics = graphics.size();
        int32_t attributes_index = scene.graphics.instances[node.graphics].attributes;
        graphics.emplace_back() = Graphics{
            .attributes = scene.graphics.attributes(attributes_index),
        };
    }

    // physics
    if (node.physics < 0) {
        result.physics = -1;
    } else {
        result.physics = physics.size();
        loader::PhysicsInstance instance = scene.physics.instances[node.physics];
        physics.emplace_back() = Physics{
            .body = instance.id,
            .trigger = {.enabled = instance.isTrigger, .action = instance.trigger.action, .argument = instance.trigger.argument},
        };
    }

    // children
    for (auto&& child : node.children) {
        int32_t child_index = convertNodes_(scene, factory, scene.get(child), index);
        result.children.push_back(child_index);
    }
    return index;
}

void Scene::callEntityInit() {
    for (auto&& ent : entities) {
        ent->init();
    }
}

void Scene::callEntityUpdate() {
    for (auto&& ent : entities) {
        ent->update();
    }
}

void Scene::callEntityPrePhysicsUpdate() {
    for (auto&& ent : entities) {
        ent->prePhysicsUpdate();
    }
}

void Scene::callEntityPostPhysicsUpdate() {
    for (auto&& ent : entities) {
        ent->postPhysicsUpdate();
    }
}

std::string SceneRef::removeTrailingNumberFromName_(std::string name) const {
    std::string result = name;
    size_t last_dot_pos = result.find_last_of('.');
    if (last_dot_pos == std::string::npos) return result;

    std::string last_part = result.substr(last_dot_pos + 1);
    bool is_numeric = std::all_of(last_part.begin(), last_part.end(), ::isdigit);
    if (is_numeric) {
        result = result.erase(last_dot_pos);
    }

    return result;
}

bool SceneRef::matchPatternAgainstName_(std::string pattern, std::string name) const {
    size_t pattern_last_pos = 0;
    size_t name_last_pos = 0;

    while (true) {
        size_t pattern_part_pos = pattern.find(".", pattern_last_pos);
        size_t name_part_pos = name.find(".", name_last_pos);

        std::string pattern_part = pattern.substr(pattern_last_pos, pattern_part_pos - pattern_last_pos);
        std::string name_part = name.substr(name_last_pos, name_part_pos - name_last_pos);

        if (pattern_part != "*" && pattern_part != name_part) return false;

        // either one is at the end
        if (pattern_part_pos == std::string::npos || name_part_pos == std::string::npos) {
            // true only if both are at the end
            return (pattern_part_pos == std::string::npos) == (name_part_pos == std::string::npos);
        }

        pattern_last_pos = pattern_part_pos + 1;
        name_last_pos = name_part_pos + 1;
    }
}

NodeRef SceneRef::find(NodeRef parent, std::string path) const {
    size_t separator_pos = path.find("/");
    std::string front = path.substr(0, separator_pos);
    std::string rest = separator_pos == std::string::npos ? "" : path.erase(0, separator_pos + 1);

    NodeRef result = {};
    for (auto&& child : parent.children()) {
        std::string non_unique_name = removeTrailingNumberFromName_(child.name());
        if (matchPatternAgainstName_(front, non_unique_name)) {
            result = child;
            break;
        }
    }

    if (result.isInvalid()) {
        PANIC("Could not find a node matching '" + path + "' from '" + parent.name() + "'");
    }

    if (rest.empty())
        return result;
    else
        return find(result, rest);
}

NodeRef NodeRef::find(const std::string& path) const {
    return SceneRef(*scene_).find(*this, path);
}

void GraphicsRef::setTransformFromNode() {
    scene_->graphics[index_].attributes->transform = scene_->transforms[node_].matrix();
}

void GraphicsRef::setTransform(const glm::mat4& matrix) {
    scene_->graphics[index_].attributes->transform = matrix;
}

}  // namespace scene
