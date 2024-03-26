#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>

#include "Loader.h"

#define TINYGLTF_IMPLEMENTATION
#undef APIENTRY
#include <tiny_gltf.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include "../Utils.h"

namespace gltf = tinygltf;

using namespace Asset;

namespace Loader {

// loads the node's transformation information into a single glm::mat4
glm::mat4 loadNodeTransform(gltf::Node &node) {
    glm::mat4 transform = glm::mat4(1.0);
    // A Node can have either a full transformation matrix or individual scale, rotation and translatin components

    // Check if matrix is present
    if (node.matrix.size() == 16) {
        transform = glm::make_mat4(node.matrix.data());
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

/**
 * @param model the entire gltf model
 * @param node the current node
 * @param meshes all of the meshes, same order as in the gltf file
 * @param instances an empty vector that will be filled with instances
 * @param attributes an empty vector that will be filled with instance attributes (e.g. transform)
 * @param combined_transform the combined transform of all parent nodes
 */
void loadNodeRecursive(gltf::Model &model, gltf::Node &node, std::vector<Mesh *> &meshes, std::vector<Instance *> &instances, std::vector<InstanceAttributes> &attributes, glm::mat4 combined_transform) {
    glm::mat4 transform = loadNodeTransform(node);
    combined_transform = combined_transform * transform;

    // Check if node has a mesh
    if (node.mesh >= 0) {
        Instance *instance = new Instance();
        instance->attributesIndex = attributes.size();
        attributes.emplace_back(InstanceAttributes{
            .transform = combined_transform,
        });
        instances.push_back(instance);
        meshes[node.mesh]->instances.push_back(instance);
    }

    for (size_t i = 0; i < node.children.size(); i++) {
        loadNodeRecursive(model, model.nodes[node.children[i]], meshes, instances, attributes, combined_transform);
    }
}

typedef struct Chunk {
    // pointer into the gltf data (vector of three floats)
    void *positionPtr = nullptr;
    // the length of the position data in bytes
    size_t positionLength;
    // pointer into the gltf data (vector of three floats)
    void *normalPtr = nullptr;
    // the length of the normal data in bytes
    size_t normalLength;
    // pointer into the gltf data (vector of two floats)
    void *texcoordPtr = nullptr;
    // the length of the texcoord data in bytes
    size_t texcoordLength;
    // pointer into the gltf data (short or int)
    void *indexPtr = nullptr;
    // the length of the index data in bytes
    size_t indexLength;
    // the size, in bytes, of each index (2 or 4)
    uint8_t indexSize;
    // then number of indices
    uint32_t elementCount;
    // the number of vertices
    uint32_t vertexCount;
    // the index of the material, negative if there is none
    int32_t material;
    // the associated section of this chunk (1:1 relationship)
    Section *section = nullptr;
} Chunk;

Mesh *loadMesh(gltf::Model &model, gltf::Mesh &mesh, std::vector<Material *> &materials, std::vector<Chunk> &chunks) {
    int first_chunk_index = chunks.size();
    Mesh *result = new Mesh();
    result->name = mesh.name;

    uint32_t total_vertex_count = 0, total_element_count = 0;
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

        if (position_access.componentType != GL_FLOAT || position_access.type != TINYGLTF_TYPE_VEC3 || position_access.sparse.isSparse || position_access.bufferView < 0) {
            std::cerr << "Primitive position attribute has invalid access" << std::endl;
            continue;
        }
        if (normal_access.componentType != GL_FLOAT || normal_access.type != TINYGLTF_TYPE_VEC3 || normal_access.sparse.isSparse || normal_access.bufferView < 0) {
            std::cerr << "Primitive normal attribute has invalid access" << std::endl;
            continue;
        }
        if (texcoord_access.componentType != GL_FLOAT || texcoord_access.type != TINYGLTF_TYPE_VEC2 || texcoord_access.sparse.isSparse || texcoord_access.bufferView < 0) {
            std::cerr << "Primitive texcoord attribute has invalid access" << std::endl;
            continue;
        }
        if ((index_access.componentType != GL_UNSIGNED_SHORT && index_access.componentType != GL_UNSIGNED_INT) || index_access.type != TINYGLTF_TYPE_SCALAR || index_access.sparse.isSparse || index_access.bufferView < 0) {
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

        uint32_t element_count = index_access.count;
        uint32_t vertex_count = position_access.count;
        chunks.emplace_back(Chunk{
            .positionPtr = &position_buffer.data.at(0) + position_view.byteOffset,
            .positionLength = position_view.byteLength,
            .normalPtr = &normal_buffer.data.at(0) + normal_view.byteOffset,
            .normalLength = normal_view.byteLength,
            .texcoordPtr = &texcoord_buffer.data.at(0) + texcoord_view.byteOffset,
            .texcoordLength = texcoord_view.byteLength,
            .indexPtr = &index_buffer.data.at(0) + index_view.byteOffset,
            .indexLength = index_view.byteLength,
            .indexSize = index_size,
            .elementCount = element_count,
            .vertexCount = vertex_count,
            .material = primitive.material,
        });

        total_element_count += element_count;
        total_vertex_count += vertex_count;
    }

    if (total_vertex_count == 0 || total_element_count == 0) {
        LOG("Mesh has no valid vertices. TODO: handle error");
    }

    // The vector must not grow / shrink, so it is initialized to the correct size
    // If it reallocates the chunk.section reference would turn invalid
    result->sections.reserve(chunks.size());
    for (size_t i = first_chunk_index; i < chunks.size(); i++) {
        Chunk &chunk = chunks[i];
        Material *material = materials.back();
        if (chunk.material >= 0) {
            material = materials[chunk.material];
        }

        result->sections.emplace_back(Section{
            .baseIndex = 0,
            .baseVertex = 0,
            .elementCount = chunk.elementCount,
            .vertexCount = chunk.vertexCount,
            .mesh = result,
            .material = material,
        });
        result->totalElementCount += chunk.elementCount;
        result->totalVertexCount += chunk.vertexCount;
        // Note: this is only save because the vector is pre-allocated
        chunk.section = &result->sections.back();
    }

    return result;
}

GL::Texture *loadTexture(const gltf::Model &model, const gltf::TextureInfo &texture_info, GLenum internalFormat) {
    if (texture_info.index < 0) {
        return nullptr;
    }
    if (texture_info.texCoord != 0) {
        LOG("only texCoord=0 is supported");
        return nullptr;
    }
    gltf::Texture texture = model.textures[texture_info.index];
    gltf::Image image = model.images[texture.source];
    if (image.bits != 8) {
        LOG("only 8-bit images are supported");
        return nullptr;
    }

    GLenum format;
    if (image.component == 1) {
        format = GL_RED;
    } else if (image.component == 2) {
        format = GL_RG;
    } else if (image.component == 3) {
        format = GL_RGB;
    } else if (image.component == 4) {
        format = GL_RGBA;
    }

    GL::Texture *result = new GL::Texture(GL_TEXTURE_2D);
    result->allocate(0, internalFormat, image.width, image.height, 1);
    result->load(0, image.width, image.height, 1, format, GL_UNSIGNED_BYTE, image.image.data());
    result->generateMipmap();
    return result;
}

Material *loadMaterial(const gltf::Model &model, gltf::Material &material) {
    Material *result = new Material();
    result->name = material.name;
    result->albedoFactor = glm::make_vec4(&material.pbrMetallicRoughness.baseColorFactor[0]);
    result->metallicRoughnessFactor = {
        material.pbrMetallicRoughness.metallicFactor,
        material.pbrMetallicRoughness.roughnessFactor,
    };

    result->albedo = loadTexture(model, material.pbrMetallicRoughness.baseColorTexture, GL_SRGB8_ALPHA8);
    result->occlusionMetallicRoughness = loadTexture(model, material.pbrMetallicRoughness.metallicRoughnessTexture, GL_RGB8);
    gltf::TextureInfo normal_info = {};
    normal_info.index = material.normalTexture.index;
    normal_info.texCoord = material.normalTexture.texCoord;
    result->normal = loadTexture(model, normal_info, GL_RGB8);

    return result;
}

Material *createDefaultMaterial() {
    Material *result = new Material();
    result->name = "Default";
    result->albedoFactor = glm::vec4(1.0);
    result->metallicRoughnessFactor = glm::vec2(0.0, 1.0);
    result->albedo = Loader::texture("assets/textures/default_albedo.png", {.srgb = true});
    result->occlusionMetallicRoughness = Loader::texture("assets/textures/default_orm.png");
    result->normal = Loader::texture("assets/textures/default_normal.png");

    return result;
}

std::shared_ptr<Asset::Scene> gltf(const std::string filename) {
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
    std::shared_ptr<Scene> result = std::make_shared<Scene>();

    std::vector<Material *> materials;
    for (size_t i = 0; i < model.materials.size(); ++i) {
        Material *material = loadMaterial(model, model.materials[i]);
        materials.push_back(material);
    }
    result->defaultMaterial = createDefaultMaterial();
    // the last material is the default one
    materials.emplace_back(result->defaultMaterial);

    uint32_t total_vertex_count = 0;
    uint32_t total_element_count = 0;
    std::vector<Mesh *> meshes;
    std::vector<Chunk> chunks;
    for (size_t i = 0; i < model.meshes.size(); ++i) {
        Mesh *mesh = loadMesh(model, model.meshes[i], materials, chunks);
        meshes.push_back(mesh);
        total_element_count += mesh->totalElementCount;
        total_vertex_count += mesh->totalVertexCount;
    }

    std::vector<InstanceAttributes> attributes;
    std::vector<Instance *> instances;
    // this may allocate a little bit more than needed, but it doesn't matter
    attributes.reserve(model.nodes.size());
    for (size_t i = 0; i < scene.nodes.size(); ++i) {
        loadNodeRecursive(model, model.nodes[scene.nodes[i]], meshes, instances, attributes, glm::mat4(1.0));
    }
    result->instances = instances;

    // sort all of the sections by material id
    // this allowes them to be drawn in larger batches
    std::sort(chunks.begin(), chunks.end(), [](Chunk const &a, Chunk const &b) {
        return a.material < b.material;
    });

    // For performance all mesh sections are concatenated into a single, large, immutable buffer
    auto position_buffer = new GL::Buffer();
    position_buffer->allocateEmpty(total_vertex_count * sizeof(glm::vec3), GL_DYNAMIC_STORAGE_BIT);
    auto normal_buffer = new GL::Buffer();
    normal_buffer->allocateEmpty(total_vertex_count * sizeof(glm::vec3), GL_DYNAMIC_STORAGE_BIT);
    auto uv_buffer = new GL::Buffer();
    uv_buffer->allocateEmpty(total_vertex_count * sizeof(glm::vec2), GL_DYNAMIC_STORAGE_BIT);
    auto element_buffer = new GL::Buffer();
    element_buffer->allocateEmpty(total_element_count * sizeof(uint16_t), GL_DYNAMIC_STORAGE_BIT);

    auto attributes_buffer = new GL::Buffer();
    attributes_buffer->allocate(attributes.data(), attributes.size() * sizeof(InstanceAttributes), 0);

    auto vao = new GL::VertexArray();
    result->vao = vao;
    vao->layout(0, 0, 3, GL_FLOAT, GL_FALSE, 0);
    vao->bindBuffer(0, *position_buffer, 0, sizeof(glm::vec3));
    vao->own(position_buffer);
    vao->layout(1, 1, 3, GL_FLOAT, GL_FALSE, 0);
    vao->bindBuffer(1, *normal_buffer, 0, sizeof(glm::vec3));
    vao->own(normal_buffer);
    vao->layout(2, 2, 2, GL_FLOAT, GL_FALSE, 0);
    vao->bindBuffer(2, *uv_buffer, 0, sizeof(glm::vec2));
    vao->own(uv_buffer);
    vao->bindElementBuffer(*element_buffer);
    vao->own(element_buffer);

    // instance attribute layout
    // 4 attributes for the 4 columns of the transformation matrix
    vao->layout(3, 3, 4, GL_FLOAT, GL_FALSE, 0 * sizeof(glm::vec4));
    vao->layout(3, 4, 4, GL_FLOAT, GL_FALSE, 1 * sizeof(glm::vec4));
    vao->layout(3, 5, 4, GL_FLOAT, GL_FALSE, 2 * sizeof(glm::vec4));
    vao->layout(3, 6, 4, GL_FLOAT, GL_FALSE, 3 * sizeof(glm::vec4));
    vao->attribDivisor(3, 1);
    vao->bindBuffer(3, *attributes_buffer, 0, sizeof(InstanceAttributes));
    vao->own(attributes_buffer);

    std::vector<GL::DrawElementsIndirectCommand> draw_commands;
    int32_t base_vertex = 0;
    uint32_t base_index = 0;
    int32_t batch_material_index = std::numeric_limits<int32_t>::max();  // just some value to mark the start
    MaterialBatch batch = {};
    for (auto &&chunk : chunks) {
        position_buffer->write(base_vertex * sizeof(glm::vec3), chunk.positionPtr, chunk.positionLength);
        normal_buffer->write(base_vertex * sizeof(glm::vec3), chunk.normalPtr, chunk.normalLength);
        uv_buffer->write(base_vertex * sizeof(glm::vec2), chunk.texcoordPtr, chunk.texcoordLength);
        element_buffer->write(base_index * chunk.indexSize, chunk.indexPtr, chunk.indexLength);
        chunk.section->baseIndex = base_index;
        chunk.section->baseVertex = base_vertex;

        // start a new batch
        if (batch_material_index != chunk.material) {
            batch_material_index = chunk.material;

            // push previous one
            if (batch.commandCount != 0)
                result->batches.emplace_back(batch);

            batch = {
                .material = chunk.material >= 0 ? materials[chunk.material] : materials.back(),
                .commandOffset = reinterpret_cast<GL::DrawElementsIndirectCommand *>(draw_commands.size() * sizeof(GL::DrawElementsIndirectCommand)),
                .commandCount = 0,
            };
        }

        uint32_t instance_count = static_cast<uint32_t>(chunk.section->mesh->instances.size());
        if (instance_count > 0) {
            uint32_t base_instance = chunk.section->mesh->instances[0]->attributesIndex;
            GL::DrawElementsIndirectCommand cmd = {
                .count = chunk.elementCount,
                .instanceCount = instance_count,
                .firstIndex = base_index,
                .baseVertex = base_vertex,
                .baseInstance = base_instance,
            };
            draw_commands.push_back(cmd);
            batch.commandCount++;
        }

        base_vertex += chunk.vertexCount;
        base_index += chunk.elementCount;
    }
    // push final one
    if (batch.commandCount != 0)
        result->batches.emplace_back(batch);

    result->drawCommandBuffer = new GL::Buffer();
    result->drawCommandBuffer->allocate(draw_commands.data(), draw_commands.size() * sizeof(GL::DrawElementsIndirectCommand), 0);

    result->materials = materials;
    result->meshes = meshes;

    return result;
}

}  // namespace Loader