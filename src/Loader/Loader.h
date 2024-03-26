#pragma once

#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "../GL/Geometry.h"
#include "../GL/Texture.h"

namespace Asset {
typedef struct Image {
    int width = 0;
    int height = 0;
    int channels = 4;
    std::shared_ptr<uint8_t> data;
} Image;

typedef struct Material {
    std::string name = "";
    // the albedo / diffuse / color texture
    GL::Texture *albedo = nullptr;
    // a combined texture. R=Occlusion, G=Metallness, B=Roughness. Note: occlusion is not implemented
    GL::Texture *occlusionMetallicRoughness = nullptr;
    // the normal texture (R=X, G=Y, B=Z)
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

struct Instance;
struct Mesh;

typedef struct Section {
    // offset of the first element index
    uint32_t baseIndex = 0;
    // offset of the first vertex (vertex_index = element_index + offset)
    uint32_t baseVertex = 0;
    // the number of elements / indices
    uint32_t elementCount = 0;
    // the number of vertices
    uint32_t vertexCount = 0;
    Mesh *mesh;
    Material *material;
} Section;

typedef struct Mesh {
    std::string name = "";
    // A mesh consits of one or multiple sections (called "primitives" by gltf).
    // Each section has only one material. If mesh has multiple materials, it will have a section for each.
    std::vector<Section> sections = {};
    // references to all the instances of this mesh
    std::vector<Instance *> instances = {};
    // the sum of all section element counts
    uint32_t totalElementCount = 0;
    // the sum of all section vertex counts
    uint32_t totalVertexCount = 0;
} Mesh;

typedef struct InstanceAttributes {
    glm::mat4 transform = glm::mat4(1);
} InstanceAttributes;

typedef struct Instance {
    std::string name = "";
    // the index into the per instance attributes buffer of this instance
    uint32_t attributesIndex;
    // reference to the mesh
    const Mesh *mesh;
} Instance;

typedef struct MaterialBatch {
    // reference to the material
    Material *material;
    // Actually an offset into the draw command buffer. OpenGL needs it as a pointer.
    // The range of commands given by the offset and count all use the same material
    GL::DrawElementsIndirectCommand *commandOffset;
    // The number of commands in this batch. Should be one per section that uses this material.
    uint32_t commandCount;
} MaterialBatch;

class Scene {
   public:
    std::string name = "";
    std::vector<Instance *> instances = {};
    std::vector<Material *> materials = {};
    Material *defaultMaterial = nullptr;
    std::vector<Mesh *> meshes = {};
    GL::VertexArray *vao = nullptr;
    std::vector<MaterialBatch> batches = {};
    GL::Buffer *drawCommandBuffer = nullptr;

    ~Scene() {
        if (vao != nullptr) {
            vao->destroy();
            vao = nullptr;
        }
        for (auto &instance : instances) delete instance;
        instances = {};
        for (auto &material : materials) delete material;
        materials = {};
        for (auto &mesh : meshes) delete mesh;
        meshes = {};
    }
};

}  // namespace Asset

namespace Loader {

std::string text(std::string filename);

std::ifstream stream(std::string filename);

std::vector<uint8_t> binary(std::string filename);

Asset::Image image(std::string filename);

struct TextureParameters {
    bool mipmap = true;
    bool srgb = false;
    GLenum internalFormat = GL_RGBA8;
    GLenum fileFormat = GL_RGBA;
    GLenum dataType = GL_UNSIGNED_BYTE;
};

GL::Texture *texture(std::string filename, TextureParameters params = {});

// References:
// https://kcoley.github.io/glTF/specification/2.0/figures/gltfOverview-2.0.0a.png
// https://github.com/KhronosGroup/glTF-Tutorials/blob/main/gltfTutorial/README.md
// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
// https://www.khronos.org/files/gltf20-reference-guide.pdf

std::shared_ptr<Asset::Scene> gltf(const std::string filename);

}  // namespace Loader
