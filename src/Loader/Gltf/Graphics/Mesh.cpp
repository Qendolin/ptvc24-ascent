#include "Graphics.h"

namespace gltf = tinygltf;

namespace loader {

Mesh &loadMesh(GraphicsLoadingContext &context, const gltf::Mesh &mesh) {
    const gltf::Model &model = context.model;
    // used at the end
    int first_chunk_index = context.chunks.size();

    Mesh &result = context.newMesh();
    result.name = mesh.name;

    uint32_t total_vertex_count = 0, total_element_count = 0, chunk_count = 0;
    for (const gltf::Primitive &primitive : mesh.primitives) {
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

        const gltf::Accessor &position_access = model.accessors[position_access_ref];
        const gltf::Accessor &normal_access = model.accessors[normal_access_ref];
        const gltf::Accessor &texcoord_access = model.accessors[texcoord_access_ref];
        const gltf::Accessor &index_access = model.accessors[primitive.indices];

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
        if (index_access.count % 3 != 0) {
            std::cerr << "Primitive index count not a multiple of three" << std::endl;
            continue;
        }

        const gltf::BufferView &position_view = model.bufferViews[position_access.bufferView];
        const gltf::BufferView &normal_view = model.bufferViews[normal_access.bufferView];
        const gltf::BufferView &texcoord_view = model.bufferViews[texcoord_access.bufferView];
        const gltf::BufferView &index_view = model.bufferViews[index_access.bufferView];

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

        const gltf::Buffer &position_buffer = model.buffers[position_view.buffer];
        const gltf::Buffer &normal_buffer = model.buffers[normal_view.buffer];
        const gltf::Buffer &texcoord_buffer = model.buffers[texcoord_view.buffer];
        const gltf::Buffer &index_buffer = model.buffers[index_view.buffer];

        uint32_t element_count = index_access.count;
        uint32_t vertex_count = position_access.count;
        Chunk &chunk = context.newChunk();
        chunk.mesh = context.meshes.size() - 1;
        chunk.positionPtr = &position_buffer.data.at(0) + position_view.byteOffset;
        chunk.positionLength = position_view.byteLength;
        chunk.normalPtr = &normal_buffer.data.at(0) + normal_view.byteOffset;
        chunk.normalLength = normal_view.byteLength;
        chunk.texcoordPtr = &texcoord_buffer.data.at(0) + texcoord_view.byteOffset;
        chunk.texcoordLength = texcoord_view.byteLength;
        chunk.indexPtr = &index_buffer.data.at(0) + index_view.byteOffset;
        chunk.indexLength = index_view.byteLength;
        chunk.indexSize = index_size;
        chunk.elementCount = element_count;
        chunk.vertexCount = vertex_count;
        chunk.material = primitive.material;

        chunk_count++;
        total_element_count += element_count;
        total_vertex_count += vertex_count;
    }

    if (total_vertex_count == 0 || total_element_count == 0) {
        LOG("Mesh has no valid vertices. TODO: handle error");
    }

    // The vector must not grow / shrink, so it is initialized to the correct size
    // If it reallocates the chunk.section reference would turn invalid
    result.sections.reserve(chunk_count);
    for (size_t i = first_chunk_index; i < first_chunk_index + chunk_count; i++) {
        Chunk &chunk = context.chunks[i];

        Section &section = result.sections.emplace_back();
        section.baseIndex = 0;
        section.baseVertex = 0;
        section.elementCount = chunk.elementCount;
        section.vertexCount = chunk.vertexCount;
        section.mesh = chunk.mesh;
        section.material = chunk.material;

        context.totalElementCount += chunk.elementCount;
        context.totalVertexCount += chunk.vertexCount;
        // Note: this is only save because the vector is pre-allocated
        chunk.section = result.sections.size() - 1;
    }

    return result;
}

}  // namespace loader