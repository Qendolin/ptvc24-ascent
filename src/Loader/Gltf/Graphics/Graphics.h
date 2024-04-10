#pragma once

#include "../../Gltf.h"

namespace loader {

/**
 * A chunk is a part of mesh with the same material.
 * It contains pointers and ranges into the gltf data,
 * which will be used when loading it into the opengl buffers.
 */
typedef struct Chunk {
    // pointer into the gltf data (vector of three floats)
    const void *positionPtr = nullptr;
    // the length of the position data in bytes
    size_t positionLength = 0;
    // pointer into the gltf data (vector of three floats)
    const void *normalPtr = nullptr;
    // the length of the normal data in bytes
    size_t normalLength = 0;
    // pointer into the gltf data (vector of two floats)
    const void *texcoordPtr = nullptr;
    // the length of the texcoord data in bytes
    size_t texcoordLength = 0;
    // pointer into the gltf data (short or int)
    const void *indexPtr = nullptr;
    // the length of the index data in bytes
    size_t indexLength = 0;
    // the size, in bytes, of each index (2 or 4)
    uint8_t indexSize = 0;
    // then number of indices
    uint32_t elementCount = 0;
    // the number of vertices
    uint32_t vertexCount = 0;
    // the index of the material, negative if there is none
    int32_t material = -1;
    // the index of the mesh, negative if there is none
    int32_t mesh = -1;
    // the associated section of this chunk (1:1 relationship)
    int32_t section = -1;
} Chunk;

/**
 * A utility class that contains all the data used during loading.
 * It is passed around the loading functions and filled in piece by piece.
 * It is tightly coupled to the implementation of the loading logic. (i.e.: parts need to be intialized in the correct order).
 *
 * The factories (`new*`) allocate objects owned by the context's vectors.
 * This should help with performance and prevent memory leaks.
 */
class GraphicsLoadingContext {
   public:
    const gltf::Model &model;
    std::map<std::string, loader::Node> &nodes;

    // all of the loaded materials
    std::vector<Material> materials;
    // index of the default material in the `materials` vector.
    int32_t defaultMaterial = -1;

    // the total number of vertices of all meshes
    uint32_t totalVertexCount = 0;
    // the total number of element incices of all meshes
    uint32_t totalElementCount = 0;
    // all of the graphics meshes
    std::vector<Mesh> meshes;
    /**
     * Since not all meshes are grahpics meshes (some are physics meshes)
     * the `meshes` vector may contain less elements than the gltf model.
     *
     * For this reason a mapping between the indices of `meshes` and gltf model is required.
     * This vector holds the index into the `meshes` array or `-1` if a gltf index does not map to any graphics mesh.
     */
    std::vector<int32_t> meshIndexMap;
    // all of the graphics mesh chunks
    std::vector<Chunk> chunks;

    // all of the graphics instances
    std::vector<Instance> instances;
    /**
     * All of the graphics instance's attributes.
     * This is a 1:1 relation, the instance and it's attribute share the same index in their vectors.
     */
    std::vector<InstanceAttributes> attributes;

    // all of the batches
    std::vector<MaterialBatch> batches;

    // the vao that references all graphics mesh data for rendering
    gl::VertexArray *vao = nullptr;
    // the vertex position buffer owen by the vao
    gl::Buffer *position = nullptr;
    // the vertex normal buffer owen by the vao
    gl::Buffer *normal = nullptr;
    // the vertex uv texture coordinate buffer owen by the vao
    gl::Buffer *uv = nullptr;
    // the element index buffer owen by the vao
    gl::Buffer *element = nullptr;
    // the per instance attributes buffer owen by the vao
    gl::Buffer *instanceAttributes = nullptr;
    // the buffer containing the commands for indirect rendering
    gl::Buffer *drawCommands = nullptr;

    GraphicsLoadingContext(const gltf::Model &model, std::map<std::string, loader::Node> &nodes) : model(model), nodes(nodes) {
    }

    // returns a newly allocated mesh
    Mesh &newMesh() {
        return meshes.emplace_back();
    }

    // returns a newly allocated mesh chunk
    Chunk &newChunk() {
        return chunks.emplace_back();
    }

    // returns a newly allocated material
    Material &newMaterial() {
        return materials.emplace_back();
    }

    // returns a newly allocated graphics instance
    Instance &newInstance() {
        return instances.emplace_back();
    }

    // returns a newly allocated graphics instance attributes
    InstanceAttributes &newInstanceAttributes() {
        return attributes.emplace_back();
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
 * Load a gltf material used for rendering
 */
Material &loadMaterial(GraphicsLoadingContext &context, const gltf::Material &material);

/**
 * Create a default / fallback material.
 */
Material &loadDefaultMaterial(GraphicsLoadingContext &context);

/**
 * Load a gltf mesh used for rendering
 */
Mesh &loadMesh(GraphicsLoadingContext &context, const gltf::Mesh &mesh);

}  // namespace loader
