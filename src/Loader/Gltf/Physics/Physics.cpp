#include "Physics.h"

#include "../../../Physics/Physics.h"
#include "../../../Physics/Shapes.h"

namespace loader {

// Create a Jolt Physics shape given it's type and size. For mesh shapes the mesh shape is also passed.
JPH::ShapeRefC createShape(PhysicsShape shape, glm::vec3 size, JPH::RefConst<JPH::MeshShapeSettings> mesh_shape) {
    JPH::ShapeSettings::ShapeResult shape_result;
    switch (shape) {
        case PhysicsShape::Box:
            shape_result = JPH::BoxShapeSettings(ph::convert(size)).Create();
            break;
        case PhysicsShape::Cylinder:
            shape_result = JPH::CylinderShapeSettings(size.y, glm::max(size.x, size.z)).Create();
            break;
        case PhysicsShape::Sphere:
            shape_result = JPH::SphereShapeSettings(glm::max(size.x, glm::max(size.y, size.z))).Create();
            break;
        case PhysicsShape::Mesh: {
            shape_result = JPH::ScaledShapeSettings(mesh_shape, ph::convert(size)).Create();
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
JPH::BodyCreationSettings createBodySettings(const PhysicsLoadingContext &context, PhysicsInstance &instance, const Node &node, const PhysicsBodyParameters &params) {
    JPH::RefConst<JPH::MeshShapeSettings> mesh_shape = nullptr;
    if (params.mesh >= 0) mesh_shape = context.meshes[params.mesh];
    auto shape = createShape(params.shape, node.initialScale, mesh_shape);

    JPH::EMotionType motion_type = JPH::EMotionType::Static;
    JPH::ObjectLayer object_layer = ph::Layers::NON_MOVING;
    if (node.isKinematic) {
        motion_type = JPH::EMotionType::Kinematic;
        object_layer = ph::Layers::MOVING;
    }
    if (instance.isTrigger) {
        object_layer = ph::Layers::SENSOR;
    }
    JPH::BodyCreationSettings settings(shape, ph::convert(node.initialPosition), ph::convert(node.initialOrientation), motion_type, object_layer);
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

PhysicsInstance &loadPhysicsInstance(PhysicsLoadingContext &context, const gltf::Node &node) {
    PhysicsInstance &result = context.newInstance();
    result.name = node.name;

    std::string trigger_string = util::getJsonValue<std::string>(node.extras, "trigger");
    if (!trigger_string.empty()) {
        std::pair<std::string, std::string> action_and_arg = parseTriggerString(trigger_string);
        result.trigger.action = action_and_arg.first;
        result.trigger.argument = action_and_arg.second;
        result.isTrigger = true;
    }

    return result;
}

PhysicsBodyParameters loadBodyParameters(PhysicsLoadingContext &context, const gltf::Node &node) {
    PhysicsBodyParameters params = {};

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

        Node &node_result = context.nodes[node.name];

        PhysicsInstance &instance = loadPhysicsInstance(context, node);
        PhysicsBodyParameters params = loadBodyParameters(context, node);
        instance.settings = createBodySettings(context, instance, node_result, params);
        instance.settings.mUserData = context.instances.size() - 1;

        node_result.physics = context.instances.size() - 1;
    });
}

Physics loadPhysics(const gltf::Model &model, std::map<std::string, loader::Node> &nodes) {
    PhysicsLoadingContext context(model, nodes);

    loadMeshes(context);

    const gltf::Scene &scene = context.model.scenes[context.model.defaultScene];
    loadInstances(context, scene);

    Physics result = {
        context.instances,
    };
    return result;
}

}  // namespace loader