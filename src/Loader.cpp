#include "Loader.h"

#include <algorithm>
#include <fstream>
#include <iostream>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#undef APIENTRY
#include <tiny_gltf.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Utils.h"

namespace gltf = tinygltf;

glm::mat4 loadNodeTransform(gltf::Node &node) {
    glm::mat4 transform = glm::mat4(1.0);
    // A Node can have either a full transformation matrix or individual scale, rotation and translatin components

    // Check if matrix is present
    if (node.matrix.size() == 16) {
        // convert doubles to floats
        std::vector<float> float_matrix(16);
        std::transform(node.matrix.begin(), node.matrix.end(), float_matrix.begin(),
                       [](double d) { return static_cast<float>(d); });
        transform = glm::make_mat4(float_matrix.data());
    } else {
        // Check if scale is present
        if (node.scale.size() == 3) {
            transform = glm::scale(transform, {node.scale[0], node.scale[1], node.scale[2]});
        }
        // Check if rotation is present
        if (node.rotation.size() == 4) {
            glm::quat quat = glm::make_quat(node.rotation.data());
            transform = glm::toMat4(quat) * transform;
        }
        // Check if translation is present
        if (node.translation.size() == 3) {
            glm::mat4 mat = glm::translate(glm::mat4(1.0), {node.translation[0], node.translation[1], node.translation[2]});
            transform = mat * transform;
        }
    }
    return transform;
}

void loadNode(gltf::Model &model, gltf::Node &node, const std::vector<Mesh> &meshes, std::vector<Instance> &instances, glm::mat4 combined_transform) {
    glm::mat4 transform = loadNodeTransform(node);
    combined_transform = combined_transform * transform;

    if (node.mesh >= 0) {
        instances.push_back(Instance{
            .name = node.name,
            .transform = combined_transform,
            .mesh = meshes[node.mesh],
        });
    }

    for (size_t i = 0; i < node.children.size(); i++) {
        loadNode(model, model.nodes[node.children[i]], meshes, instances, combined_transform);
    }
}

Mesh loadMesh(gltf::Model &model, gltf::Mesh &mesh) {
    Mesh result = {};
    result.name = mesh.name;
    result.vao = new GL::VertexArray();

    struct Block {
        void *positionPtr;
        size_t positionLength;
        void *normalPtr;
        size_t normalLength;
        void *texcoordPtr;
        size_t texcoordLength;
        void *indexPtr;
        size_t indexLength;
        uint8_t indexSize;
        uint32_t vertexCount;
        int32_t material;
    };

    std::vector<struct Block> primitives;

    uint32_t total_vertex_count = 0;
    for (size_t i = 0; i < mesh.primitives.size(); ++i) {
        gltf::Primitive &primitive = mesh.primitives[i];
        if (primitive.mode != TINYGLTF_MODE_TRIANGLES) {
            std::cerr << "Unsupported primitive mode " << std::to_string(primitive.mode) << std::endl;
            continue;
        }

        int position_access_ref = -1;
        int normal_access_ref = -1;
        int texcoord_access_ref = -1;

        for (auto &attrib : primitive.attributes) {
            std::string key = attrib.first;

            if (key.compare("POSITION") == 0) {
                position_access_ref = attrib.second;
            } else if (attrib.first.compare("NORMAL") == 0) {
                normal_access_ref = attrib.second;
            } else if (attrib.first.compare("TEXCOORD_0") == 0) {
                texcoord_access_ref = attrib.second;
            }
        }

        if (position_access_ref < 0 || normal_access_ref < 0 || texcoord_access_ref < 0) {
            std::cerr << "Primitive is missing a required attribute" << std::endl;
            continue;
        }

        gltf::Accessor &position_access = model.accessors[position_access_ref];
        gltf::Accessor &normal_access = model.accessors[normal_access_ref];
        gltf::Accessor &texcoord_access = model.accessors[texcoord_access_ref];
        gltf::Accessor &index_access = model.accessors[primitive.indices];

        if (position_access.componentType != GL_FLOAT || position_access.type != TINYGLTF_TYPE_VEC3 || position_access.sparse.isSparse) {
            std::cerr << "Primitive position attribute has invalid access" << std::endl;
            continue;
        }
        if (normal_access.componentType != GL_FLOAT || normal_access.type != TINYGLTF_TYPE_VEC3 || normal_access.sparse.isSparse) {
            std::cerr << "Primitive normal attribute has invalid access" << std::endl;
            continue;
        }
        if (texcoord_access.componentType != GL_FLOAT || texcoord_access.type != TINYGLTF_TYPE_VEC2 || texcoord_access.sparse.isSparse) {
            std::cerr << "Primitive texcoord attribute has invalid access" << std::endl;
            continue;
        }
        if ((index_access.componentType != GL_UNSIGNED_SHORT && index_access.componentType != GL_UNSIGNED_INT) || index_access.type != TINYGLTF_TYPE_SCALAR || index_access.sparse.isSparse) {
            std::cerr << "Primitive index has invalid access" << std::endl;
            continue;
        }

        gltf::BufferView &position_view = model.bufferViews[position_access.bufferView];
        gltf::BufferView &normal_view = model.bufferViews[normal_access.bufferView];
        gltf::BufferView &texcoord_view = model.bufferViews[texcoord_access.bufferView];
        gltf::BufferView &index_view = model.bufferViews[index_access.bufferView];

        if (position_view.target != GL_ARRAY_BUFFER || position_view.byteStride != 0) {
            std::cerr << "Primitive position attribute has invalid view" << std::endl;
            continue;
        }
        if (normal_view.target != GL_ARRAY_BUFFER || normal_view.byteStride != 0) {
            std::cerr << "Primitive normal attribute has invalid view" << std::endl;
            continue;
        }
        if (texcoord_view.target != GL_ARRAY_BUFFER || texcoord_view.byteStride != 0) {
            std::cerr << "Primitive texcoord attribute has invalid view" << std::endl;
            continue;
        }
        if (index_view.target != GL_ELEMENT_ARRAY_BUFFER || index_view.byteStride != 0) {
            std::cerr << "Primitive index has invalid view" << std::endl;
            continue;
        }

        uint8_t index_size = 0;
        if (index_access.componentType == GL_UNSIGNED_SHORT) index_size = 2;
        if (index_access.componentType == GL_UNSIGNED_INT) index_size = 4;

        // TODO: support int and short indices
        if (index_size != 2) {
            std::cerr << "Primitive does not have short indices" << std::endl;
            continue;
        }

        gltf::Buffer &position_buffer = model.buffers[position_view.buffer];
        gltf::Buffer &normal_buffer = model.buffers[normal_view.buffer];
        gltf::Buffer &texcoord_buffer = model.buffers[texcoord_view.buffer];
        gltf::Buffer &index_buffer = model.buffers[index_view.buffer];

        uint32_t vertex_count = index_access.count;
        primitives.push_back(Block{
            .positionPtr = &position_buffer.data.at(0) + position_view.byteOffset,
            .positionLength = position_view.byteLength,
            .normalPtr = &normal_buffer.data.at(0) + normal_view.byteOffset,
            .normalLength = normal_view.byteLength,
            .texcoordPtr = &texcoord_buffer.data.at(0) + texcoord_view.byteOffset,
            .texcoordLength = texcoord_view.byteLength,
            .indexPtr = &index_buffer.data.at(0) + index_view.byteOffset,
            .indexLength = index_view.byteLength,
            .indexSize = index_size,
            .vertexCount = vertex_count,
            .material = primitive.material,
        });

        total_vertex_count += vertex_count;
    }

    result.positions = new GL::Buffer();
    result.positions->allocateEmpty(total_vertex_count * sizeof(glm::vec3), GL_DYNAMIC_STORAGE_BIT);
    result.normals = new GL::Buffer();
    result.normals->allocateEmpty(total_vertex_count * sizeof(glm::vec3), GL_DYNAMIC_STORAGE_BIT);
    result.uvs = new GL::Buffer();
    result.uvs->allocateEmpty(total_vertex_count * sizeof(glm::vec2), GL_DYNAMIC_STORAGE_BIT);
    result.indices = new GL::Buffer();
    result.indices->allocateEmpty(total_vertex_count * sizeof(uint16_t), GL_DYNAMIC_STORAGE_BIT);

    uint32_t base = 0;
    for (auto &p : primitives) {
        result.positions->write(base * sizeof(glm::vec3), p.positionPtr, p.positionLength);
        result.normals->write(base * sizeof(glm::vec3), p.normalPtr, p.normalLength);
        result.uvs->write(base * sizeof(glm::vec2), p.texcoordPtr, p.texcoordLength);
        result.indices->write(base * p.indexSize, p.indexPtr, p.indexLength);

        result.sections.push_back(Section{
            .base = base,
            .length = p.vertexCount,
            .material = p.material,
        });

        base += p.vertexCount;
    }

    result.vao->layout(0, 0, 3, GL_FLOAT, GL_FALSE, 0);
    result.vao->bindBuffer(0, *result.positions, 0, sizeof(glm::vec3));
    result.vao->layout(1, 1, 3, GL_FLOAT, GL_FALSE, 0);
    result.vao->bindBuffer(1, *result.normals, 0, sizeof(glm::vec3));
    result.vao->layout(2, 2, 2, GL_FLOAT, GL_FALSE, 0);
    result.vao->bindBuffer(2, *result.uvs, 0, sizeof(glm::vec2));

    result.vao->layoutI(3, 3, 1, GL_UNSIGNED_SHORT, 0);
    result.vao->bindElementBuffer(*result.indices);

    return result;
}

std::vector<Instance> loadModel(const std::string filename) {
    gltf::TinyGLTF loader;
    std::string err;
    std::string warn;
    gltf::Model model;

    LOG("Loading GLTF: " << filename);

    std::string ext = filename.substr(filename.find_last_of("."));
    bool res = false;
    if (ext == ".glb") {
        res = loader.LoadBinaryFromFile(&model, &err, &warn, filename);
    } else if (ext == ".gltf") {
        res = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
    } else {
        PANIC("Unknown file extension " + ext);
    }

    if (!warn.empty()) {
        std::cerr << "WARN: " << warn << std::endl;
    }

    if (!err.empty()) {
        std::cerr << "ERROR: " << err << std::endl;
    }

    if (!res) {
        PANIC("Failed to load glTF: " + filename);
    }

    const gltf::Scene &scene = model.scenes[model.defaultScene];

    std::vector<Mesh> meshes;
    for (size_t i = 0; i < model.meshes.size(); ++i) {
        const Mesh mesh = loadMesh(model, model.meshes[i]);
        meshes.push_back(mesh);
    }

    std::vector<Instance> instances;
    for (size_t i = 0; i < scene.nodes.size(); ++i) {
        loadNode(model, model.nodes[scene.nodes[i]], meshes, instances, glm::mat4(1.0));
    }
    return instances;
}