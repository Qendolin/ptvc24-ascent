#pragma once

#include <glm/glm.hpp>

#include "GL/Geometry.h"
#include "GL/Texture.h"

// References:
// https://kcoley.github.io/glTF/specification/2.0/figures/gltfOverview-2.0.0a.png
// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html

typedef struct Section {
    uint32_t base;
    uint32_t length;
    // TODO:
    int32_t material;
} Section;

typedef struct Mesh {
    std::string name;
    GL::VertexArray *vao;
    GL::Buffer *positions;
    GL::Buffer *normals;
    GL::Buffer *uvs;
    GL::Buffer *indices;
    std::vector<Section> sections;
} Mesh;

typedef struct Instance {
    std::string name;
    glm::mat4 transform;
    const Mesh mesh;
} Instance;

std::vector<Instance> loadModel(const std::string filename);