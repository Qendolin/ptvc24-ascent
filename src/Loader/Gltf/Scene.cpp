#include <sstream>

#include "../Gltf.h"

// later
#include "../../Physics/Physics.h"
#include "../../Physics/Shapes.h"

namespace gltf = tinygltf;

using namespace Asset;

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

}  // namespace Loader