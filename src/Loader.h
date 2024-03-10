#pragma once

#include <glm/glm.hpp>

#include "GL/Geometry.h"
#include "GL/Texture.h"

// References:
// https://kcoley.github.io/glTF/specification/2.0/figures/gltfOverview-2.0.0a.png
// https://github.com/KhronosGroup/glTF-Tutorials/blob/main/gltfTutorial/README.md
// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html

typedef struct Material {
    std::string name = "";
    GL::Texture *albedo = nullptr;
    GL::Texture *metallicRoughness = nullptr;
    GL::Texture *normal = nullptr;
    glm::vec3 albedoFactor = {1, 1, 1};
    float metallicFactor = 0;
    float roughnessFactor = 1;
} Material;

typedef struct Section {
    uint32_t base = 0;
    uint32_t length = 0;
    const Material material;
} Section;

typedef struct Mesh {
    std::string name = "";
    GL::VertexArray *vao = nullptr;
    GL::Buffer *positions = nullptr;
    GL::Buffer *normals = nullptr;
    GL::Buffer *uvs = nullptr;
    GL::Buffer *indices = nullptr;
    std::vector<Section> sections = {};
} Mesh;

typedef struct Instance {
    std::string name = "";
    glm::mat4 transform = glm::mat4(1);
    const Mesh mesh;
} Instance;

std::vector<Instance> loadModel(const std::string filename);