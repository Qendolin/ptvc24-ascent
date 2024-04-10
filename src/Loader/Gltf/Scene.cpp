#include <sstream>

#include "../Gltf.h"

// later
#include "../../Physics/Physics.h"
#include "../../Physics/Shapes.h"

namespace gltf = tinygltf;

namespace Loader {

// loads the node's transformation information into a single glm::mat4
glm::mat4 loadNodeTransform(const gltf::Node &node) {
    glm::mat4 transform = glm::mat4(1.0);
    // A Node can have either a full transformation matrix or individual scale, rotation and translatin components

    // Check if matrix is present
    if (node.matrix.size() == 16) {
        transform = glm::make_mat4(node.matrix.data());
    } else {
        // Check if scale is present
        if (node.scale.size() == 3) {
            transform = glm::scale(transform, {node.scale[0], node.scale[1], node.scale[2]});
        }
        // Check if rotation is present
        if (node.rotation.size() == 4) {
            glm::quat quat = glm::make_quat(node.rotation.data());
            transform = glm::toMat4(quat) * transform;
        }
        // Check if translation is present
        if (node.translation.size() == 3) {
            glm::mat4 mat = glm::translate(glm::mat4(1.0), {node.translation[0], node.translation[1], node.translation[2]});
            transform = mat * transform;
        }
    }
    return transform;
}

void loadNodesRecursive(const gltf::Model &model, const gltf::Node &node, glm::mat4 combined_transform, NodeConsumer consumer) {
    glm::mat4 transform = loadNodeTransform(node);
    combined_transform = combined_transform * transform;

    consumer(node, combined_transform);

    for (size_t i = 0; i < node.children.size(); i++) {
        loadNodesRecursive(model, model.nodes[node.children[i]], combined_transform, consumer);
    }
}

void loadNodes(const gltf::Model &model, const gltf::Scene &scene, NodeConsumer consumer) {
    for (int node_ref : scene.nodes) {
        loadNodesRecursive(model, model.nodes[node_ref], glm::mat4(1.0), consumer);
    }
}

/**
 * `value` can either be one of the primitve types:
 *  `bool`, `int`, `float`, `string` or an object reference.
 */
std::any jsonPropertyAsAny(const gltf::Value &value) {
    if (value.IsBool()) return value.Get<bool>();
    if (value.IsString()) return value.Get<std::string>();
    if (value.IsInt()) return value.Get<int>();
    if (value.IsReal()) return static_cast<float>(value.Get<double>());
    if (value.IsObject()) {
        if (value.Has("type")) {
            auto type_value = value.Get("type");
            auto type_string = type_value.IsString() ? type_value.Get<std::string>() : "";
            // A blender object reference
            if (type_string == "Object") {
                return value.Get("name").Get<std::string>();
            }
        }
    }
    return std::any();
}

void assignNodeParents(std::map<std::string, Node> &nodes, Node &node, std::string parent) {
    node.parent = parent;
    for (auto &&child : node.children) {
        assignNodeParents(nodes, nodes[child], node.name);
    }
}

// Parse a string `<tag> ',' <tag> ',' ...` to a vector of tags
std::vector<std::string> parseTagsString(const std::string &str) {
    std::vector<std::string> parts;
    std::stringstream ss(str);
    std::string part;
    while (std::getline(ss, part, ',')) {
        // Remove leading and trailing whitespaces
        part.erase(part.begin(), std::find_if(part.begin(), part.end(), [](unsigned char c) { return !std::isspace(c); }));
        part.erase(std::find_if(part.rbegin(), part.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), part.end());
        parts.push_back(part);
    }
    return parts;
}

void decomposeTransform(const glm::mat4 &transform, glm::vec3 *translation, glm::quat *rotation, glm::vec3 *scale) {
    // https://math.stackexchange.com/a/1463487/1014081
    // calculate scale
    *scale = {glm::length(transform[0]), glm::length(transform[1]), glm::length(transform[2])};
    // calculate rotaton
    glm::mat3 rotation_mat = transform * glm::diagonal4x4(glm::vec4(1.0 / scale->x, 1.0 / scale->y, 1.0 / scale->z, 1.0));

    *translation = transform[3];
    *rotation = glm::quat_cast(rotation_mat);
}

std::map<std::string, Loader::Node> loadNodeTree(const gltf::Model &model) {
    std::map<std::string, Node> nodes;
    auto &scene = model.scenes[model.defaultScene];

    nodes["#root"] = {.name = "#root"};
    std::vector<std::string> root_children;
    root_children.reserve(scene.nodes.size());
    std::transform(scene.nodes.begin(), scene.nodes.end(), std::back_inserter(root_children), [&model](int node_ref) {
        return model.nodes[node_ref].name;
    });
    nodes["#root"].children = root_children;

    loadNodes(model, scene, [&model, &nodes](const gltf::Node &node, const glm::mat4 &transform) {
        std::vector<std::string> children;
        children.reserve(node.children.size());
        std::transform(node.children.begin(), node.children.end(), std::back_inserter(children), [&model](int node_ref) {
            return model.nodes[node_ref].name;
        });

        // all properties starting with "prop." are collected into the map
        std::map<std::string, std::any> properties;
        if (node.extras.IsObject()) {
            for (std::string &key : node.extras.Keys()) {
                if (key.starts_with("prop.")) {
                    std::string short_key = key.substr(5);  // remove "prop." prefix
                    properties[short_key] = jsonPropertyAsAny(node.extras.Get(key));
                }
            }
        }

        bool kinematic = Util::getJsonValue<bool>(node.extras, "kinematic");

        std::vector<std::string> tags;
        std::string tags_string = Util::getJsonValue<std::string>(node.extras, "tags");
        if (!tags_string.empty()) {
            tags = parseTagsString(tags_string);
        }

        glm::vec3 translation;
        glm::quat rotation;
        glm::vec3 scale;
        decomposeTransform(transform, &translation, &rotation, &scale);

        nodes[node.name] = {
            .name = node.name,
            .children = std::move(children),
            .initialTransform = transform,
            .initialPosition = translation,
            .initialScale = scale,
            .initialOrientation = rotation,
            .entityClass = Util::getJsonValue<std::string>(node.extras, "entity"),
            .properties = std::move(properties),
            .tags = std::move(tags),
            .isKinematic = kinematic,
        };
    });

    assignNodeParents(nodes, nodes["#root"], "");

    return nodes;
}

}  // namespace Loader