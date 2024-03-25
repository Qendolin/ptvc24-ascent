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
    GL::Texture *albedo = nullptr;
    GL::Texture *occlusionMetallicRoughness = nullptr;
    GL::Texture *normal = nullptr;
    glm::vec3 albedoFactor = {1, 1, 1};
    glm::vec2 metallicRoughnessFactor = {0, 1};
} Material;

struct Instance;
struct Mesh;

typedef struct Section {
    uint32_t baseIndex = 0;
    uint32_t baseVertex = 0;
    uint32_t elementCount = 0;
    uint32_t vertexCount = 0;
    const Mesh *mesh;
    const Material *material;
} Section;

typedef struct Mesh {
    std::string name = "";
    std::vector<Section> sections = {};
    std::vector<const Instance *> instances = {};
    uint32_t totalElementCount = 0;
    uint32_t totalVertexCount = 0;
} Mesh;

typedef struct InstanceAttributes {
    glm::mat4 transform = glm::mat4(1);
} InstanceAttributes;

typedef struct Instance {
    std::string name = "";
    uint32_t attributesIndex;
    const Mesh *mesh;
} Instance;

typedef struct MaterialBatch {
    const Material *material;
    GL::DrawElementsIndirectCommand *commandOffset;
    uint32_t commandCount;
} MaterialBatch;

typedef struct Scene {
    std::string name = "";
    std::vector<Instance *> instances = {};
    GL::VertexArray *vao = nullptr;
    std::vector<MaterialBatch> batches = {};
    GL::Buffer *drawCommandBuffer = nullptr;
} Scene;

}  // namespace Asset

namespace Loader {

std::string text(std::string filename);

std::ifstream stream(std::string filename);

std::vector<uint8_t> binary(std::string filename);

Asset::Image image(std::string filename);

GL::Texture *texture(std::string filename);

// References:
// https://kcoley.github.io/glTF/specification/2.0/figures/gltfOverview-2.0.0a.png
// https://github.com/KhronosGroup/glTF-Tutorials/blob/main/gltfTutorial/README.md
// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
// https://www.khronos.org/files/gltf20-reference-guide.pdf

Asset::Scene gltf(const std::string filename);

}  // namespace Loader
