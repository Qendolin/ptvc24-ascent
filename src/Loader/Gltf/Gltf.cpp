#include "../Gltf.h"

#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>

#include "../../GL/Geometry.h"
#include "../../GL/Texture.h"
#include "../../Util/Log.h"

namespace gltf = tinygltf;

namespace loader {

Material::~Material() {
    delete albedo;
    delete occlusionMetallicRoughness;
    delete normal;
}

const gltf::Model gltf(const std::string filename) {
    gltf::TinyGLTF loader;
    gltf::Model model;
    std::string err;
    std::string warn;

    LOG_INFO("Loading GLTF: " + filename);

    std::string ext = filename.substr(filename.find_last_of("."));
    bool ok = false;
    if (ext == ".glb") {
        ok = loader.LoadBinaryFromFile(&model, &err, &warn, filename);
    } else if (ext == ".gltf") {
        ok = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
    } else {
        PANIC("Unknown gltf file extension '" + ext + "', should be .gltf or .glb");
    }

    if (!warn.empty()) {
        LOG_WARN("Warning: " + warn);
    }

    if (!err.empty()) {
        LOG_WARN("Error: " + err);
    }

    if (!ok) {
        PANIC("Failed to load glTF: " + filename);
    }
    return model;
}

GraphicsData::GraphicsData(
    std::vector<Instance> &&instances,
    std::vector<Material> &&materials,
    int32_t default_material,
    std::vector<Mesh> &&meshes,
    std::vector<MaterialBatch> &&batches,
    gl::VertexArray *vao,
    gl::Buffer *instance_attributes,
    gl::Buffer *draw_commands)
    : instances(std::move(instances)),
      materials(std::move(materials)),
      defaultMaterial(this->materials[default_material]),
      meshes(std::move(meshes)),
      batches(std::move(batches)),
      vao_(vao),
      drawCommands_(draw_commands) {
    instanceAttributesData_ = instance_attributes->mapRange<InstanceAttributes>(GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
}

GraphicsData::~GraphicsData() = default;

uint32_t GraphicsData::commandCount() const {
    return drawCommands_->size() / sizeof(gl::DrawElementsIndirectCommand);
}

void GraphicsData::bind() const {
    vao_->bind();
    drawCommands_->bind(GL_DRAW_INDIRECT_BUFFER);
}

InstanceAttributes *GraphicsData::attributes(int32_t index) const {
    return &instanceAttributesData_[index];
}

PhysicsData::PhysicsData(std::vector<PhysicsInstance> &instances) : instances(std::move(instances)) {}

PhysicsData::~PhysicsData() = default;

SceneData *scene(const gltf::Model &model) {
    std::map<std::string, loader::Node> nodes = loadNodeTree(model);
    GraphicsData g = loadGraphics(model, nodes);
    PhysicsData ph = loadPhysics(model, nodes);
    std::string name = model.scenes[model.defaultScene].name;
    return new SceneData(name, std::move(g), std::move(ph), std::move(nodes));
}

namespace util {

template <>
std::string getJsonValue<std::string>(const gltf::Value &object, const std::string &key) {
    if (!object.IsObject()) return "";
    gltf::Value element = object.Get(key);
    if (!element.IsString()) return "";
    return element.Get<std::string>();
}

template <>
bool getJsonValue<bool>(const gltf::Value &object, const std::string &key) {
    if (!object.IsObject()) return false;
    gltf::Value element = object.Get(key);
    if (!element.IsBool()) return false;
    return element.Get<bool>();
}

}  // namespace util

SceneData::SceneData(std::string name, loader::GraphicsData &&graphics, loader::PhysicsData &&physics, std::map<std::string, loader::Node> &&nodes)
    : name(name),
      graphics(std::move(graphics)),
      physics(std::move(physics)),
      nodes_(std::move(nodes)),
      root_(nodes_.at("#root")) {
}

}  // namespace loader
