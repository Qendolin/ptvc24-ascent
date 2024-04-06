#include "Physics.h"

using namespace Asset;

namespace Loader {

// Create a Jolt Physics shape given it's type and size. For mesh shapes the mesh shape is also passed.
JPH::ShapeRefC createShape(PhysicsShape shape, glm::vec3 size, JPH::RefConst<JPH::MeshShapeSettings> mesh_shape) {
    JPH::ShapeSettings::ShapeResult shape_result;
    switch (shape) {
        case PhysicsShape::Box:
            shape_result = JPH::BoxShapeSettings(PH::convert(size)).Create();
            break;
        case PhysicsShape::Cylinder:
            shape_result = JPH::CylinderShapeSettings(size.y, glm::max(size.x, size.z)).Create();
            break;
        case PhysicsShape::Sphere:
            shape_result = JPH::SphereShapeSettings(glm::max(size.x, glm::max(size.y, size.z))).Create();
            break;
        case PhysicsShape::Mesh: {
            shape_result = JPH::ScaledShapeSettings(mesh_shape, PH::convert(size)).Create();
            break;
        }
        default:
            PANIC("Invalid shape");
    }

    if (shape_result.HasError())
        PANIC(static_cast<std::string>(shape_result.GetError()));
    return shape_result.Get();
}

// Create Jolt Body settings, ready to be added to the Jolt simulatin.
JPH::BodyCreationSettings createBodySettings(const PhysicsLoadingContext &context, PhysicsInstance &instance, const PhysicsBodyParameters &params) {
    JPH::RefConst<JPH::MeshShapeSettings> mesh_shape = nullptr;
    if (params.mesh >= 0) mesh_shape = context.meshes[params.mesh];
    auto shape = createShape(params.shape, params.size, mesh_shape);

    JPH::EMotionType motion_type = JPH::EMotionType::Static;
    JPH::ObjectLayer object_layer = PH::Layers::NON_MOVING;
    if (instance.isKinematic) {
        motion_type = JPH::EMotionType::Kinematic;
        object_layer = PH::Layers::MOVING;
    }
    if (instance.isTrigger) {
        object_layer = PH::Layers::SENSOR;
    }
    JPH::BodyCreationSettings settings(shape, PH::convert(params.position), PH::convert(params.orientation), motion_type, object_layer);
    settings.mIsSensor = instance.isTrigger;
    return settings;
}

// Parse a string `<action> | <action> ':' <argument>` into the action and argument.
std::pair<std::string, std::string> parseTriggerString(const std::string &str) {
    size_t pos = str.find(':');
    if (pos == std::string::npos) {
        // If delimiter not found, return the whole string as first part and an empty string as second part
        return std::make_pair(str, "");
    }
    return std::make_pair(str.substr(0, pos), str.substr(pos + 1));
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

// Get a string from a json object or `""` if it is not possible
std::string getString(const tinygltf::Value &object, const std::string &key) {
    if (!object.IsObject()) return "";
    gltf::Value element = object.Get(key);
    if (!element.IsString()) return "";
    return element.Get<std::string>();
}

// Get a bool from a json object or `false` if it is not possible
bool getBool(const tinygltf::Value &object, const std::string &key) {
    if (!object.IsObject()) return false;
    gltf::Value element = object.Get(key);
    if (!element.IsBool()) return false;
    return element.Get<bool>();
}

PhysicsInstance &loadPhysicsInstance(PhysicsLoadingContext &context, const gltf::Node &node, const glm::mat4 &transform) {
    PhysicsInstance &result = context.newInstance();
    result.name = node.name;

    std::string trigger_string = getString(node.extras, "trigger");
    if (!trigger_string.empty()) {
        std::pair<std::string, std::string> action_and_arg = parseTriggerString(trigger_string);
        result.trigger.action = action_and_arg.first;
        result.trigger.argument = action_and_arg.second;
        result.isTrigger = true;
    }
    std::string tags_string = getString(node.extras, "tags");
    if (!tags_string.empty()) {
        result.tags = parseTagsString(tags_string);
    }
    result.isKinematic = getBool(node.extras, "kinematic");

    return result;
}

PhysicsBodyParameters loadBodyParameters(PhysicsLoadingContext &context, const gltf::Node &node, const glm::mat4 &transform) {
    PhysicsBodyParameters params = {};

    // https://math.stackexchange.com/a/1463487/1014081
    // calculate scale
    glm::vec3 scale = {glm::length(transform[0]), glm::length(transform[1]), glm::length(transform[2])};
    // calculate rotaton
    glm::mat3 rotation_mat = transform * glm::diagonal4x4(glm::vec4(1.0 / scale.x, 1.0 / scale.y, 1.0 / scale.z, 1.0));

    params.size = scale;
    params.position = transform[3];
    params.orientation = glm::quat_cast(rotation_mat);

    if (node.mesh >= 0) {
        params.mesh = context.mapMeshIndex(node.mesh);
        params.shape = PhysicsShape::Mesh;
    }

    if (node.name.contains("Box"))
        params.shape = PhysicsShape::Box;
    else if (node.name.contains("Cylinder"))
        params.shape = PhysicsShape::Cylinder;
    else if (node.name.contains("Sphere"))
        params.shape = PhysicsShape::Sphere;

    return params;
}

void loadMeshes(PhysicsLoadingContext &context) {
    context.meshIndexMap.reserve(context.model.meshes.size());
    for (const gltf::Mesh &gltf_mesh : context.model.meshes) {
        if (!gltf_mesh.name.starts_with("Phys")) {
            context.addMeshIndex(-1);
            continue;
        }

        JPH::RefConst<JPH::MeshShapeSettings> mesh = loadMesh(context, gltf_mesh);
        context.addMeshIndex(context.meshes.size() - 1);
    }
}

void loadInstances(PhysicsLoadingContext &context, const gltf::Scene &scene) {
    loadNodes(context.model, scene, [&](const gltf::Node &node, const glm::mat4 &transform) {
        if (!node.name.starts_with("Phys")) return;

        PhysicsInstance &instance = loadPhysicsInstance(context, node, transform);
        PhysicsBodyParameters params = loadBodyParameters(context, node, transform);
        instance.settings = createBodySettings(context, instance, params);
        instance.settings.mUserData = context.instances.size() - 1;
    });
}

Physics physics(const gltf::Model &model) {
    PhysicsLoadingContext context(model);

    loadMeshes(context);

    const gltf::Scene &scene = context.model.scenes[context.model.defaultScene];
    loadInstances(context, scene);

    Physics result = {
        context.instances,
    };
    return result;
}

}  // namespace Loader