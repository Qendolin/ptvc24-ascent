
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

#include "../Physics/Physics.h"
#include "../Physics/Shapes.h"
#include "../Utils.h"
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
typedef struct Material {
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

    ~Material() {
        if (albedo != nullptr) {
            albedo->destroy();
            albedo = nullptr;
        }
        if (occlusionMetallicRoughness != nullptr) {
            occlusionMetallicRoughness->destroy();
            occlusionMetallicRoughness = nullptr;
        }
        if (normal != nullptr) {
            normal->destroy();
            normal = nullptr;
        }
    }
} Material;

/**
 * Part of a graphics mesh that has the same material.
 * Does not actually contain any mesh data.
 * See "Chunk" for details.
 */
typedef struct Section {
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
} Section;

/**
 * A graphics mesh, made up of `Sections`.
 * Does not actually contain any mesh data.
 */
typedef struct Mesh {
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
} Mesh;

/**
 * Per graphics instance attributes.
 * Currently only the mesh's transform.
 * Accessible by the vertex shader.
 */
typedef struct InstanceAttributes {
    // the object to world transformation matrix
    glm::mat4 transform = glm::mat4(1);
} InstanceAttributes;

/**
 * A graphics instance.
 * Represents a node in the gltf file which has a mesh that can be rendered.
 */
typedef struct Instance {
    std::string name = "";
    // index of the per instance attributes
    int32_t attributes = -1;
    // index of the mesh
    int32_t mesh = -1;
} Instance;

/**
 * A range draw commands that all use the same material.
 * Used for indirect drawing.
 */
typedef struct MaterialBatch {
    // reference to the material
    int32_t material = -1;
    // Actually an offset into the draw command buffer. OpenGL needs it as a pointer.
    // The range of commands given by the offset and count all use the same material
    gl::DrawElementsIndirectCommand *commandOffset;
    // The number of commands in this batch. Should be one per section that uses this material.
    uint32_t commandCount;
} MaterialBatch;

/**
 * Contains all the graphics instances and their related data.
 */
class Graphics {
   private:
    std::unique_ptr<gl::VertexArray> vao_ = nullptr;
    std::unique_ptr<gl::Buffer> drawCommands_ = nullptr;

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

    Graphics(Graphics const &) = delete;
    Graphics &operator=(Graphics const &) = delete;
    Graphics(Graphics &&) noexcept = default;

    Graphics(
        std::vector<Instance> &&instances,
        std::vector<Material> &&materials,
        int32_t default_material,
        std::vector<Mesh> &&meshes,
        std::vector<MaterialBatch> &&batches,
        gl::VertexArray *vao,
        gl::Buffer *instance_attributes,
        gl::Buffer *draw_commands);

    ~Graphics();

    // bind the vao and draw commands
    void bind() const;

    InstanceAttributes *attributes(int32_t index) const;
};

// Definition of physics trigger (sensor) action.
typedef struct Trigger {
    std::string action = "";
    std::string argument = "";
} Trigger;

/**
 * A physics instance.
 * Represents a node in the gltf file which has a physics body.
 */
typedef struct PhysicsInstance {
    std::string name = "";
    bool isTrigger = false;
    Trigger trigger = {};

    JPH::BodyID id = JPH::BodyID();
    JPH::BodyCreationSettings settings;
} PhysicsInstance;

/**
 * Contains all the physics instances.
 */
class Physics {
   public:
    std::vector<PhysicsInstance> instances;

    Physics(Physics const &) = delete;
    Physics &operator=(Physics const &) = delete;
    Physics(Physics &&) = default;

    Physics(std::vector<PhysicsInstance> &instances) : instances(std::move(instances)) {}

    // TODO: this is temporary
    void create(ph::Physics &physics) {
        for (size_t i = 0; i < instances.size(); i++) {
            PhysicsInstance &instance = instances[i];
            JPH::BodyID id = physics.interface().CreateAndAddBody(instance.settings, JPH::EActivation::DontActivate);
            if (!instance.id.IsInvalid()) PANIC("Instance already has a physics body id");
            instance.id = id;
        }
    }
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
class Scene {
   private:
    std::map<std::string, loader::Node> nodes_;
    const loader::Node &root_;

   public:
    std::string name = "";

    loader::Graphics graphics;
    loader::Physics physics;

    Scene(Scene const &) = delete;
    Scene &operator=(Scene const &) = delete;

    Scene(std::string name, loader::Graphics &&graphics, loader::Physics &&physics, std::map<std::string, loader::Node> &&nodes);

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
typedef std::function<void(const gltf::Node &, const glm::mat4 &)> NodeConsumer;

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
Graphics loadGraphics(const gltf::Model &model, std::map<std::string, loader::Node> &nodes);

/**
 * Load physics instances from the gltf model.
 * @param model the gltf model
 * @param nodes the loaded node hierarchy
 */
Physics loadPhysics(const gltf::Model &model, std::map<std::string, loader::Node> &nodes);

std::map<std::string, loader::Node> loadNodeTree(const gltf::Model &model);

Scene *scene(const gltf::Model &model);

namespace util {

template <typename T>
T getJsonValue(const gltf::Value &object, const std::string &key);

}  // namespace util

}  // namespace loader