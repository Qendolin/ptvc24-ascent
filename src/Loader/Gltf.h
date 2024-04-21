
#pragma once

#include <tiny_gltf.h>

#include <algorithm>
#include <any>
#include <fstream>
#include <functional>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_operation.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>
#include <limits>
#include <ranges>

#include "../GL/Declarations.h"
#include "../GL/Indirect.h"
#include "../Physics/Physics.h"
#include "../Physics/Shapes.h"
#include "Loader.h"

namespace gltf = tinygltf;

// References:
// https://kcoley.github.io/glTF/specification/2.0/figures/gltfOverview-2.0.0a.png
// https://github.com/KhronosGroup/glTF-Tutorials/blob/main/gltfTutorial/README.md
// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
// https://www.khronos.org/files/gltf20-reference-guide.pdf

namespace loader {

/**
 * A material determines how an object looks by defining PBR parameters
 * like color and roughness.
 *
 * A material cannot be copied, only moved, because it owns it's associated textures.
 */
struct Material {
    // prevent copy
    Material(Material const &) = delete;
    Material &operator=(Material const &) = delete;
    // allow move (required for use in vector, see https://stackoverflow.com/a/67537699/7448536)
    Material(Material &&) noexcept = default;

    Material() = default;

    std::string name = "";
    // the albedo / diffuse / color texture owned by the material
    gl::Texture *albedo = nullptr;
    // a combined texture. R=Occlusion, G=Metallness, B=Roughness owned by the material. Note: occlusion is not implemented
    gl::Texture *occlusionMetallicRoughness = nullptr;
    // the normal texture (R=X, G=Y, B=Z) owned by the material
    gl::Texture *normal = nullptr;
    // a multipicative factor for the albedo color
    glm::vec3 albedoFactor = {1, 1, 1};
    // a multipicative factor for metallness and roughness
    glm::vec2 metallicRoughnessFactor = {0, 1};
    // scale normal x and y in tangent space, affects the intensity of the normal map
    float normalFactor = 1;

    ~Material();
};

/**
 * Part of a graphics mesh that has the same material.
 * Does not actually contain any mesh data.
 * See "Chunk" for details.
 */
struct Section {
    // offset of the first element index
    uint32_t baseIndex = 0;
    // offset of the first vertex (vertex_index = element_index + offset)
    uint32_t baseVertex = 0;
    // the number of elements / indices
    uint32_t elementCount = 0;
    // the number of vertices
    uint32_t vertexCount = 0;

    // index of the mesh
    int32_t mesh = -1;
    // index of the material
    int32_t material = -1;
};

/**
 * A graphics mesh, made up of `Sections`.
 * Does not actually contain any mesh data.
 */
struct Mesh {
    std::string name = "";
    // A mesh consits of one or multiple sections (called "primitives" by gltf).
    // Each section has only one material. If mesh has multiple materials, it will have a section for each.
    std::vector<Section> sections = {};
    // indices to all the instances of this mesh
    std::vector<int32_t> instances = {};
    // the sum of all section element counts
    uint32_t totalElementCount = 0;
    // the sum of all section vertex counts
    uint32_t totalVertexCount = 0;
};

/**
 * Per graphics instance attributes.
 * Currently only the mesh's transform.
 * Accessible by the vertex shader.
 */
struct InstanceAttributes {
    // the object to world transformation matrix
    glm::mat4 transform = glm::mat4(1);
};

/**
 * A graphics instance.
 * Represents a node in the gltf file which has a mesh that can be rendered.
 */
struct Instance {
    std::string name = "";
    // index of the per instance attributes
    int32_t attributes = -1;
    // index of the mesh
    int32_t mesh = -1;
};

/**
 * A range draw commands that all use the same material.
 * Used for indirect drawing.
 */
struct MaterialBatch {
    // reference to the material
    int32_t material = -1;
    // Actually an offset into the draw command buffer. OpenGL needs it as a pointer.
    // The range of commands given by the offset and count all use the same material
    gl::DrawElementsIndirectCommand *commandOffset;
    // The number of commands in this batch. Should be one per section that uses this material.
    uint32_t commandCount;
};

/**
 * Contains all the graphics instances and their related data.
 */
class GraphicsData {
   private:
    std::unique_ptr<gl::VertexArray> vao_;
    std::unique_ptr<gl::Buffer> drawCommands_;

    /**
     * Pointer into the persistently mapped, instance attributes buffer.
     * Can only be written to, not read.
     */
    InstanceAttributes *instanceAttributesData_ = nullptr;

   public:
    std::vector<Instance> instances;
    std::vector<Material> materials;
    const Material &defaultMaterial;
    std::vector<Mesh> meshes;
    std::vector<MaterialBatch> batches;

    GraphicsData(GraphicsData const &) = delete;
    GraphicsData &operator=(GraphicsData const &) = delete;
    GraphicsData(GraphicsData &&) noexcept = default;

    GraphicsData(
        std::vector<Instance> &&instances,
        std::vector<Material> &&materials,
        int32_t default_material,
        std::vector<Mesh> &&meshes,
        std::vector<MaterialBatch> &&batches,
        gl::VertexArray *vao,
        gl::Buffer *instance_attributes,
        gl::Buffer *draw_commands);

    ~GraphicsData();

    // bind the vao and draw commands
    void bind() const;

    InstanceAttributes *attributes(int32_t index) const;
};

// Definition of physics trigger (sensor) action.
struct Trigger {
    std::string action = "";
    std::string argument = "";
};

/**
 * A physics instance.
 * Represents a node in the gltf file which has a physics body.
 */
struct PhysicsInstance {
    std::string name = "";
    bool isTrigger = false;
    Trigger trigger = {};

    JPH::BodyID id = JPH::BodyID();
    JPH::BodyCreationSettings settings;
};

/**
 * Contains all the physics instances.
 */
class PhysicsData {
   public:
    std::vector<PhysicsInstance> instances;

    PhysicsData(PhysicsData const &) = delete;
    PhysicsData &operator=(PhysicsData const &) = delete;
    PhysicsData(PhysicsData &&) = default;

    PhysicsData(std::vector<PhysicsInstance> &instances);
    ~PhysicsData();
};

/**
 * A node in gltf file.
 * It is part of a node tree hierarchy.
 * It has at least name and a transformation.
 * It may have an associated graphics or physics instance.
 */
struct Node {
    // names are unique
    std::string name = "";
    // index of the graphics instance or -1
    int32_t graphics = -1;
    // index of the physics instance or -1
    int32_t physics = -1;
    // names of the child nodes
    std::vector<std::string> children;
    // name of the parent node
    std::string parent = "";

    // transform of the node when it was loaded (world space)
    glm::mat4 initialTransform = glm::mat4(1.0);
    // position of the node when it was loaded (world space)
    glm::vec3 initialPosition = glm::vec3(0.0);
    // scale of the node when it was loaded (world space)
    glm::vec3 initialScale = glm::vec3(0.0);
    // orientation / rotation of the node when it was loaded (world space)
    glm::quat initialOrientation = glm::quat();

    // identifier of the entity class
    std::string entityClass = "";
    // custom properties, contains arbitrary key value pairs
    std::map<std::string, std::any> properties;
    // custom tags, contains arbitrary strings
    std::vector<std::string> tags = {};

    // if the node is kinematic (moveable) or static
    bool isKinematic = false;

    bool isRoot() { return name == "#root"; }
};

/**
 * Represents a scene loaded from a gltf file.
 * It contains a hierarchy of nodes and the associated physics and graphics.
 */
class SceneData {
   private:
    std::map<std::string, loader::Node> nodes_;
    const loader::Node &root_;

   public:
    std::string name = "";

    loader::GraphicsData graphics;
    loader::PhysicsData physics;

    SceneData(SceneData const &) = delete;
    SceneData &operator=(SceneData const &) = delete;

    SceneData(std::string name, loader::GraphicsData &&graphics, loader::PhysicsData &&physics, std::map<std::string, loader::Node> &&nodes);

    // @return the root node
    const loader::Node &root() const {
        return root_;
    }

    // @return the node given it's name
    const loader::Node &get(std::string name) const {
        return nodes_.at(name);
    }

    // @return all nodes
    const auto all() const {
        return std::views::values(nodes_);
    }

    // @return number of all nodes
    size_t count() const {
        return nodes_.size();
    }
};

/**
 * @param node the node
 * @param transform the node's world transformation matrix
 */
typedef std::function<void(const gltf::Node &node, const glm::mat4 &transform)> NodeConsumer;

/**
 * Load a scene's node hierarchy
 *
 * @param model the entire gltf model
 * @param scene the scene of which the nodes should be loaded
 * @param consumer a function that will be called for every node in the scene
 */
void loadNodes(const gltf::Model &model, const gltf::Scene &scene, NodeConsumer consumer);

// Load a gltf file. Note: gltf::Model refers to the entire file structure, not a single 3D model.
const gltf::Model gltf(const std::string filename);

/**
 * Load graphics instances from the gltf model.
 * @param model the gltf model
 * @param nodes the loaded node hierarchy
 */
GraphicsData loadGraphics(const gltf::Model &model, std::map<std::string, loader::Node> &nodes);

/**
 * Load physics instances from the gltf model.
 * @param model the gltf model
 * @param nodes the loaded node hierarchy
 */
PhysicsData loadPhysics(const gltf::Model &model, std::map<std::string, loader::Node> &nodes);

std::map<std::string, loader::Node> loadNodeTree(const gltf::Model &model);

SceneData *scene(const gltf::Model &model);

namespace util {

template <typename T>
T getJsonValue(const gltf::Value &object, const std::string &key);

}  // namespace util

}  // namespace loader