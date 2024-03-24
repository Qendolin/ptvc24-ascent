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

std::vector<Asset::Instance> gltf(const std::string filename);

}  // namespace Loader
