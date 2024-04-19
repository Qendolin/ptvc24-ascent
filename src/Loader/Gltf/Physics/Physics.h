#pragma once

#include "../../Gltf.h"

namespace loader {

enum class PhysicsShape {
    None,
    Box,
    Sphere,
    Cylinder,
    Mesh
};

struct PhysicsBodyParameters {
    PhysicsShape shape = PhysicsShape::None;
    // if the shape is a mesh shape this will be the index of the mesh
    int32_t mesh = -1;
};

/**
 * A utility class that contains all the data used during loading.
 * It is passed around the loading functions and filled in piece by piece.
 * It is tightly coupled to the implementation of the loading logic. (i.e.: parts need to be intialized in the correct order).
 *
 * The factories (`new*`) allocate objects owned by the context's vectors.
 * This should help with performance and prevent memory leaks.
 */
class PhysicsLoadingContext {
   public:
    const gltf::Model &model;
    std::map<std::string, loader::Node> &nodes;

    std::vector<JPH::RefConst<JPH::MeshShapeSettings>> meshes;
    /**
     * Since not all meshes are physics meshes (some are graphics meshes)
     * the `meshes` vector may contain less elements than the gltf model.
     *
     * For this reason a mapping between the indices of `meshes` and gltf model is required.
     * This vector holds the index into the `meshes` array or `-1` if a gltf index does not map to any graphics mesh.
     */
    std::vector<int32_t> meshIndexMap;

    // all of the graphics instances
    std::vector<PhysicsInstance> instances;

    PhysicsLoadingContext(const gltf::Model &model, std::map<std::string, loader::Node> &nodes) : model(model), nodes(nodes) {
    }

    // returns a newly allocated physics instance
    PhysicsInstance &newInstance() {
        return instances.emplace_back();
    }

    /**
     * @param index a gltf mesh reference
     * @return the mapped index into the `meshes` vector
     */
    int32_t mapMeshIndex(int32_t index) {
        return meshIndexMap[index];
    }

    /**
     * Adds an index mapping for a gltf mesh reference.
     * @param index the mapped index into the `meshes` vector
     */
    void addMeshIndex(int32_t index) {
        meshIndexMap.push_back(index);
    }
};

/**
 * Load a gltf mesh used for physics collision
 */
JPH::RefConst<JPH::MeshShapeSettings> loadMesh(PhysicsLoadingContext &context, const gltf::Mesh &mesh);

}  // namespace loader