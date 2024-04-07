
#pragma once

#include <tiny_gltf.h>

#include <algorithm>
#include <fstream>
#include <functional>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_operation.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>
#include <limits>

#include "../Physics/Physics.h"
#include "../Physics/Shapes.h"
#include "../Utils.h"
#include "Loader.h"

namespace gltf = tinygltf;

using namespace Asset;

// References:
// https://kcoley.github.io/glTF/specification/2.0/figures/gltfOverview-2.0.0a.png
// https://github.com/KhronosGroup/glTF-Tutorials/blob/main/gltfTutorial/README.md
// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
// https://www.khronos.org/files/gltf20-reference-guide.pdf

namespace Loader {

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
    GL::Texture *albedo = nullptr;
    // a combined texture. R=Occlusion, G=Metallness, B=Roughness owned by the material. Note: occlusion is not implemented
    GL::Texture *occlusionMetallicRoughness = nullptr;
    // the normal texture (R=X, G=Y, B=Z) owned by the material
    GL::Texture *normal = nullptr;
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
 * Per (mesh) instance attributes.
 * Currently only the mesh's transform.
 */
typedef struct InstanceAttributes {
    glm::mat4 transform = glm::mat4(1);
} InstanceAttributes;

/**
 * A mesh instance.
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
    GL::DrawElementsIndirectCommand *commandOffset;
    // The number of commands in this batch. Should be one per section that uses this material.
    uint32_t commandCount;
} MaterialBatch;

// TODO: comment
class Graphics {
   private:
    std::unique_ptr<GL::VertexArray> vao_ = nullptr;
    std::unique_ptr<GL::Buffer> drawCommands_ = nullptr;

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
        GL::VertexArray *vao,
        GL::Buffer *draw_commands)
        : instances(std::move(instances)),
          materials(std::move(materials)),
          defaultMaterial(this->materials[default_material]),
          meshes(std::move(meshes)),
          batches(std::move(batches)),
          vao_(vao),
          drawCommands_(draw_commands) {
    }

    void bind() const {
        vao_->bind();
        drawCommands_->bind(GL_DRAW_INDIRECT_BUFFER);
    }

    ~Graphics() {
        if (vao_ != nullptr) {
            // I admit, this is shit
            auto tmp = vao_.release();
            tmp->destroy();
            vao_ = nullptr;
        }
        if (drawCommands_ != nullptr) {
            auto tmp = drawCommands_.release();
            tmp->destroy();
            drawCommands_ = nullptr;
        }
    }
};

// Definition of physics trigger (sensor) action.
typedef struct Trigger {
    std::string action = "";
    std::string argument = "";
} Trigger;

// TODO: comment
typedef struct PhysicsInstance {
    std::string name = "";
    std::vector<std::string> tags = {};
    bool isTrigger = false;
    Trigger trigger = {};
    bool isKinematic = false;

    JPH::BodyCreationSettings settings;
} PhysicsInstance;

// TODO: comment
class Physics {
   public:
    std::vector<PhysicsInstance> instances;

    Physics(Physics const &) = delete;
    Physics &operator=(Physics const &) = delete;
    Physics(Physics &&) = default;

    Physics(std::vector<PhysicsInstance> &instances) : instances(std::move(instances)) {}

    // TODO: this is temporary
    void create(PH::Physics &physics) {
        for (size_t i = 0; i < instances.size(); i++) {
            const PhysicsInstance &instance = instances[i];
            JPH::BodyID id = physics.interface().CreateAndAddBody(instance.settings, JPH::EActivation::DontActivate);

            if (instance.isTrigger) {
                physics.contactListener->RegisterCallback(id, [&](PH::SensorContact contact) {
                    if (contact.persistent) return;
                    size_t index = physics.interface().GetUserData(contact.sensor);
                    const Trigger &trigger = instances[index].trigger;
                    LOG("Triggered: " + trigger.action + "(" + trigger.argument + ")");
                });
            }
        }
    }
};

}  // namespace Loader

namespace Asset {

// TODO: comment
class Scene {
   public:
    std::string name = "";

    Loader::Graphics graphics;
    Loader::Physics physics;

    Scene(Scene const &) = delete;
    Scene &operator=(Scene const &) = delete;

    Scene(std::string name, Loader::Graphics &&graphics, Loader::Physics &&physics)
        : name(name),
          graphics(std::move(graphics)),
          physics(std::move(physics)) {
    }
};

}  // namespace Asset

namespace Loader {

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

Graphics graphics(const gltf::Model &model);

Physics physics(const gltf::Model &model);

Scene *scene(const gltf::Model &model);

}  // namespace Loader