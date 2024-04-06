#include "Physics.h"

namespace gltf = tinygltf;

using namespace Asset;

namespace Loader {

JPH::RefConst<JPH::MeshShapeSettings> loadMesh(PhysicsLoadingContext &context, const gltf::Mesh &mesh) {
    const gltf::Model &model = context.model;

    JPH::VertexList vertices;
    JPH::IndexedTriangleList triangles;

    if (mesh.primitives.size() > 1) {
        // Just a warning
        std::cerr << "Physics Mesh contains more than one primitive" << std::endl;
    }

    for (const gltf::Primitive &primitive : mesh.primitives) {
        if (primitive.mode != TINYGLTF_MODE_TRIANGLES) {
            std::cerr << "Unsupported primitive mode " << std::to_string(primitive.mode) << std::endl;
            continue;
        }

        int position_access_ref = -1;
        for (auto &attrib : primitive.attributes) {
            if (attrib.first.compare("POSITION") == 0) {
                position_access_ref = attrib.second;
                break;
            }
        }

        if (position_access_ref < 0) {
            std::cerr << "Primitive is missing a position attribute" << std::endl;
            continue;
        }

        const gltf::Accessor &position_access = model.accessors[position_access_ref];
        const gltf::Accessor &index_access = model.accessors[primitive.indices];
        if (position_access.componentType != GL_FLOAT || position_access.type != TINYGLTF_TYPE_VEC3 || position_access.sparse.isSparse || position_access.bufferView < 0) {
            std::cerr << "Primitive position attribute has invalid access" << std::endl;
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
        const gltf::BufferView &index_view = model.bufferViews[index_access.bufferView];
        if (position_view.target != GL_ARRAY_BUFFER || position_view.byteStride != 0) {
            std::cerr << "Primitive position attribute has invalid view" << std::endl;
            continue;
        }
        if (index_view.target != GL_ELEMENT_ARRAY_BUFFER || index_view.byteStride != 0) {
            std::cerr << "Primitive index has invalid view" << std::endl;
            continue;
        }

        const gltf::Buffer &position_buffer = model.buffers[position_view.buffer];
        const gltf::Buffer &index_buffer = model.buffers[index_view.buffer];
        uint32_t triangle_count = index_access.count / 3;
        uint32_t vertex_count = position_access.count;

        triangles.reserve(triangles.size() + triangle_count);
        vertices.reserve(vertices.size() + vertex_count);

        // Copy data from buffer to result
        // not sure if this is the best way to do it.
        const uint8_t *data_ptr = position_buffer.data.data() + position_view.byteOffset;
        for (size_t i = 0; i < vertex_count; ++i) {
            JPH::Float3 vertex;
            static_assert(sizeof(vertex) == sizeof(float[3]), "size of JPH::Float3 does not match three packed floats");
            std::memcpy(&vertex, data_ptr + i * sizeof(vertex), sizeof(vertex));
            vertices.push_back(vertex);
        }

        data_ptr = index_buffer.data.data() + index_view.byteOffset;
        for (size_t i = 0; i < triangle_count; ++i) {
            // I can't figure out a proper way to read the data
            // This might have issues with endianess, but i don't know
            static_assert(std::endian::native == std::endian::little, "not little endian architecture");
            JPH::IndexedTriangle &triangle = triangles.emplace_back(0, 0, 0);
            if (index_access.componentType == GL_UNSIGNED_SHORT) {
                triangle.mIdx[0] = data_ptr[i * 6 + 0] | (data_ptr[i * 6 + 1] << 8);
                triangle.mIdx[1] = data_ptr[i * 6 + 2] | (data_ptr[i * 6 + 3] << 8);
                triangle.mIdx[2] = data_ptr[i * 6 + 4] | (data_ptr[i * 6 + 5] << 8);
            } else {
                triangle.mIdx[0] = data_ptr[i * 12 + 0] | (data_ptr[i * 6 + 1] << 8) | (data_ptr[i * 6 + 2] << 16) | (data_ptr[i * 6 + 3] << 24);
                triangle.mIdx[1] = data_ptr[i * 12 + 4] | (data_ptr[i * 6 + 5] << 8) | (data_ptr[i * 6 + 6] << 16) | (data_ptr[i * 6 + 7] << 24);
                triangle.mIdx[2] = data_ptr[i * 12 + 8] | (data_ptr[i * 6 + 9] << 8) | (data_ptr[i * 6 + 10] << 16) | (data_ptr[i * 6 + 11] << 24);
            }
        }
    }

    JPH::Ref<JPH::MeshShapeSettings> result = new JPH::MeshShapeSettings(vertices, triangles);
    context.meshes.push_back(result);
    return result;
}

}  // namespace Loader