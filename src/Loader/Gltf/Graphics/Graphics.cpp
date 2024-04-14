
#include "Graphics.h"

#include "../../../GL/Geometry.h"

namespace gltf = tinygltf;

namespace loader {

void createVertexBuffers(GraphicsLoadingContext &context) {
    // For performance all mesh sections are concatenated into a single, large, immutable buffer

    uint32_t vertex_count = context.totalVertexCount;
    uint32_t element_count = context.totalElementCount;

    // positions
    auto position_buffer = new gl::Buffer();
    context.position = position_buffer;
    position_buffer->setDebugLabel("gltf/vbo/position");
    position_buffer->allocateEmpty(vertex_count * sizeof(glm::vec3), GL_DYNAMIC_STORAGE_BIT);

    context.vao->layout(0, 0, 3, GL_FLOAT, GL_FALSE, 0);
    context.vao->bindBuffer(0, *position_buffer, 0, sizeof(glm::vec3));
    context.vao->own(position_buffer);

    // normals
    auto normal_buffer = new gl::Buffer();
    context.normal = normal_buffer;
    normal_buffer->setDebugLabel("gltf/vbo/normal");
    normal_buffer->allocateEmpty(vertex_count * sizeof(glm::vec3), GL_DYNAMIC_STORAGE_BIT);

    context.vao->layout(1, 1, 3, GL_FLOAT, GL_FALSE, 0);
    context.vao->bindBuffer(1, *normal_buffer, 0, sizeof(glm::vec3));
    context.vao->own(normal_buffer);

    // uvs
    auto uv_buffer = new gl::Buffer();
    context.uv = uv_buffer;
    uv_buffer->setDebugLabel("gltf/vbo/uv");
    uv_buffer->allocateEmpty(vertex_count * sizeof(glm::vec2), GL_DYNAMIC_STORAGE_BIT);

    context.vao->layout(2, 2, 2, GL_FLOAT, GL_FALSE, 0);
    context.vao->bindBuffer(2, *uv_buffer, 0, sizeof(glm::vec2));
    context.vao->own(uv_buffer);

    // element indices
    auto element_buffer = new gl::Buffer();
    context.element = element_buffer;
    element_buffer->setDebugLabel("gltf/ebo");
    element_buffer->allocateEmpty(element_count * sizeof(uint16_t), GL_DYNAMIC_STORAGE_BIT);

    context.vao->bindElementBuffer(*element_buffer);
    context.vao->own(element_buffer);
}

void createInstanceAttributesBuffer(GraphicsLoadingContext &context) {
    context.instanceAttributes = new gl::Buffer();
    context.instanceAttributes->setDebugLabel("gltf/vbo/instance_attributes");
    context.instanceAttributes->allocate(context.attributes.data(), context.attributes.size() * sizeof(InstanceAttributes), GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

    // instance attribute layout
    // 4 attributes for the 4 columns of the transformation matrix
    context.vao->layout(3, 3, 4, GL_FLOAT, GL_FALSE, 0 * sizeof(glm::vec4));
    context.vao->layout(3, 4, 4, GL_FLOAT, GL_FALSE, 1 * sizeof(glm::vec4));
    context.vao->layout(3, 5, 4, GL_FLOAT, GL_FALSE, 2 * sizeof(glm::vec4));
    context.vao->layout(3, 6, 4, GL_FLOAT, GL_FALSE, 3 * sizeof(glm::vec4));
    context.vao->attribDivisor(3, 1);
    context.vao->bindBuffer(3, *context.instanceAttributes, 0, sizeof(InstanceAttributes));
    context.vao->own(context.instanceAttributes);
}

void createBatches(GraphicsLoadingContext &context) {
    // sort all of the sections by material id
    // this allowes them to be drawn in larger batches
    std::sort(context.chunks.begin(), context.chunks.end(), [](Chunk const &a, Chunk const &b) {
        return a.material < b.material;
    });

    int32_t base_vertex = 0;
    uint32_t base_index = 0;
    int32_t batch_material_index = std::numeric_limits<int32_t>::max();  // just some value to mark the start
    std::vector<gl::DrawElementsIndirectCommand> draw_commands;
    MaterialBatch batch = {};
    for (auto &&chunk : context.chunks) {
        context.position->write(base_vertex * sizeof(glm::vec3), chunk.positionPtr, chunk.positionLength);
        context.normal->write(base_vertex * sizeof(glm::vec3), chunk.normalPtr, chunk.normalLength);
        context.uv->write(base_vertex * sizeof(glm::vec2), chunk.texcoordPtr, chunk.texcoordLength);
        context.element->write(base_index * chunk.indexSize, chunk.indexPtr, chunk.indexLength);

        Mesh &mesh = context.meshes[chunk.mesh];
        Section &section = mesh.sections[chunk.section];

        section.baseIndex = base_index;
        section.baseVertex = base_vertex;

        // start a new batch
        if (batch_material_index != chunk.material) {
            batch_material_index = chunk.material;

            // push previous one
            if (batch.commandCount != 0)
                context.batches.push_back(batch);

            // convert the index into a byte offset, becaue opengl requires it
            auto command_offset = reinterpret_cast<gl::DrawElementsIndirectCommand *>(draw_commands.size() * sizeof(gl::DrawElementsIndirectCommand));
            batch = {
                .material = chunk.material,
                .commandOffset = command_offset,
                .commandCount = 0,
            };
        }

        // Add draw command
        uint32_t instance_count = static_cast<uint32_t>(mesh.instances.size());
        if (instance_count > 0) {
            Instance first_instance = context.instances[mesh.instances[0]];
            uint32_t base_instance = first_instance.attributes;
            gl::DrawElementsIndirectCommand cmd = {
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
        context.batches.emplace_back(batch);

    context.drawCommands = new gl::Buffer();
    context.drawCommands->setDebugLabel("gltf/command_buffer");
    context.drawCommands->allocate(draw_commands.data(), draw_commands.size() * sizeof(gl::DrawElementsIndirectCommand), 0);
}

void loadMaterials(GraphicsLoadingContext &context) {
    context.materials.reserve(context.model.materials.size() + 1);

    for (const gltf::Material &gltf_material : context.model.materials) {
        loadMaterial(context, gltf_material);
    }

    // Default / fallback material
    loadDefaultMaterial(context);
}

void loadMeshes(GraphicsLoadingContext &context) {
    context.meshIndexMap.reserve(context.model.meshes.size());

    for (const gltf::Mesh &gltf_mesh : context.model.meshes) {
        if (gltf_mesh.name.starts_with("Phys")) {
            context.addMeshIndex(-1);
            continue;
        }

        loadMesh(context, gltf_mesh);
        context.addMeshIndex(context.meshes.size() - 1);
    }
}

void loadInstances(GraphicsLoadingContext &context, const gltf::Scene &scene) {
    // this may allocate a little bit more than needed, but it doesn't matter
    context.attributes.reserve(context.model.nodes.size());

    loadNodes(context.model, scene, [&](const gltf::Node &node, const glm::mat4 &transform) {
        if (node.name.starts_with("Phys")) return;
        if (node.mesh < 0) return;

        InstanceAttributes &attributes = context.newInstanceAttributes();
        attributes.transform = transform;

        Instance &instance = context.newInstance();
        instance.name = node.name;
        instance.attributes = context.attributes.size() - 1;
        instance.mesh = context.mapMeshIndex(node.mesh);

        Mesh &mesh = context.meshes[instance.mesh];
        mesh.instances.push_back(context.instances.size() - 1);

        context.nodes[node.name].graphics = context.instances.size() - 1;
    });
}

GraphicsData loadGraphics(const gltf::Model &model, std::map<std::string, loader::Node> &nodes) {
    GraphicsLoadingContext context(model, nodes);

    loadMaterials(context);
    loadMeshes(context);

    const gltf::Scene &scene = context.model.scenes[context.model.defaultScene];
    loadInstances(context, scene);

    context.vao = new gl::VertexArray();
    context.vao->setDebugLabel("gltf/vao");
    createVertexBuffers(context);
    createInstanceAttributesBuffer(context);

    createBatches(context);

    GraphicsData result(
        std::move(context.instances),
        std::move(context.materials),
        context.defaultMaterial,
        std::move(context.meshes),
        std::move(context.batches),
        context.vao,
        context.instanceAttributes,
        context.drawCommands);
    return result;
}

}  // namespace loader