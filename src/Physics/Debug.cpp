#include "Debug.h"

#include <iostream>

#include "../GL/StateManager.h"

namespace PH {

// Callback for traces
void traceCallback(const char *fmt, ...) {
    // Format the message
    std::va_list list;
    va_start(list, fmt);
    char buffer[1024];
    std::vsnprintf(buffer, sizeof(buffer), fmt, list);
    va_end(list);

    std::cout << buffer << std::endl;
}

#ifdef JPH_ENABLE_ASSERTS
// Callback for asserts, connect this to your own assert handler if you have one
bool assertFailedCallback(const char *expression, const char *message, const char *file, uint32_t line) {
    std::cout << file << ":" << line << ": (" << expression << ") " << (message != nullptr ? message : "") << std::endl;

    // Breakpoint
    return true;
};
#endif  // JPH_ENABLE_ASSERTS

#ifdef JPH_DEBUG_RENDERER
DebugRendererImpl::DebugRendererImpl() {
    vao_ = new GL::VertexArray();
    vao_->setDebugLabel("physics/debug/vao");
    // Position
    vao_->layout(0, 0, 3, GL_FLOAT, false, 0);
    // Normal
    vao_->layout(0, 1, 3, GL_FLOAT, false, 3 * sizeof(float));
    // UV
    vao_->layout(0, 2, 2, GL_FLOAT, false, 6 * sizeof(float));
    // Color
    vao_->layout(0, 3, 4, GL_UNSIGNED_BYTE, true, 8 * sizeof(float));

    vbo_ = new GL::Buffer();
    vbo_->setDebugLabel("physics/debug/vbo");
    vbo_->allocateEmptyMutable(sizeof(Vertex) * 8192, GL_DYNAMIC_DRAW);
    ebo_ = new GL::Buffer();
    ebo_->setDebugLabel("physics/debug/ebo");
    ebo_->allocateEmptyMutable(sizeof(uint32_t) * 8192, GL_DYNAMIC_DRAW);

    vao_->bindBuffer(0, *vbo_, 0, sizeof(Vertex));
    vao_->bindElementBuffer(*ebo_);

    shader_ = new GL::ShaderPipeline({new GL::ShaderProgram("assets/shaders/physics_debug.vert"),
                                      new GL::ShaderProgram("assets/shaders/physics_debug.frag")});
    shader_->setDebugLabel("physics/debug/shader");
    DebugRenderer::Initialize();
}

DebugRendererImpl::~DebugRendererImpl() {
    vao_->destroy();
    vbo_->destroy();
    ebo_->destroy();
    shader_->destroy();
}

void DebugRendererImpl::DrawLine(JPH::RVec3Arg from, JPH::RVec3Arg to, JPH::ColorArg color) {
    // TODO: implement
    return;
}

void DebugRendererImpl::DrawTriangle(JPH::RVec3Arg v1, JPH::RVec3Arg v2, JPH::RVec3Arg v3,
                                     JPH::ColorArg color, ECastShadow castShadow) {
    // TODO: implement
    return;
}

JPH::DebugRenderer::Batch DebugRendererImpl::CreateTriangleBatch(const Triangle *triangles, int triangle_count) {
    std::vector<Vertex> vertices;
    vertices.reserve(triangle_count * 3);
    std::vector<uint32_t> indices;
    indices.reserve(triangle_count * 3);

    for (size_t i = 0; i < triangle_count; i++) {
        const Triangle &triangle = triangles[i];
        vertices.emplace_back(triangle.mV[0]);
        vertices.emplace_back(triangle.mV[1]);
        vertices.emplace_back(triangle.mV[2]);
        indices.emplace_back(i * 3 + 0);
        indices.emplace_back(i * 3 + 1);
        indices.emplace_back(i * 3 + 2);
    }

    return CreateTriangleBatch(vertices.data(), vertices.size(), indices.data(), indices.size());
}

JPH::DebugRenderer::Batch DebugRendererImpl::CreateTriangleBatch(const Vertex *vertices, int vertex_count,
                                                                 const uint32_t *indices, int index_count) {
    size_t length = vertex_count * sizeof(Vertex);
    size_t vertex = vboWriteOffset_ / sizeof(Vertex);
    vbo_->grow(vboWriteOffset_ + length);
    vbo_->write(vboWriteOffset_, vertices, length);
    vboWriteOffset_ += length;

    size_t index = eboWriteOffset_ / sizeof(uint32_t);
    BatchImpl *batch = new BatchImpl(index, vertex, index_count);

    length = index_count * sizeof(uint32_t);
    ebo_->grow(eboWriteOffset_ + length);
    ebo_->write(eboWriteOffset_, indices, length);
    eboWriteOffset_ += length;

    return batch;
}

void DebugRendererImpl::DrawGeometry(JPH::RMat44Arg model_matrix,
                                     const JPH::AABox &world_space_bounds,
                                     float lod_scale_sq, JPH::ColorArg model_color,
                                     const GeometryRef &geometry, ECullMode cull_mode,
                                     ECastShadow cast_shadow,
                                     EDrawMode draw_mode) {
    Batch batch = geometry->mLODs[0].mTriangleBatch;
    if (batch.GetPtr() == nullptr) return;

    BatchImpl *batch_impl = static_cast<BatchImpl *>(batch.GetPtr());
    const glm::mat4 &glm_model_matrix = reinterpret_cast<const glm::mat4 &>(model_matrix);
    JPH::Vec4 model_color_vec = model_color.ToVec4();
    const glm::vec4 glm_color_vec = {model_color_vec.GetX(), model_color_vec.GetY(), model_color_vec.GetZ(), model_color_vec.GetW()};

    drawQueue_.push_back(DrawCommand{
        .batch = batch_impl,
        .modelMatrix = glm_model_matrix,
        .modelColor = glm_color_vec,
        .cullMode = cull_mode,
        .drawMode = draw_mode,
    });
}

void DebugRendererImpl::DrawText3D(JPH::RVec3Arg position,
                                   const JPH::string_view &string,
                                   JPH::ColorArg color, float height) {
    // Not implemented
}

void DebugRendererImpl::Draw(glm::mat4 view_projection_matrix) {
    GL::pushDebugGroup("JoltDebugRenderer::Draw");
    shader_->vertexStage()->setUniform("u_view_projection_mat", view_projection_matrix);

    GL::manager->setEnabled({GL::Capability::DepthTest, GL::Capability::Blend, GL::Capability::CullFace});
    GL::manager->depthFunc(GL::DepthFunc::GreaterOrEqual);
    GL::manager->blendEquation(GL::BlendEquation::FuncAdd);
    GL::manager->blendFunc(GL::BlendFactor::SrcAlpha, GL::BlendFactor::OneMinusSrcAlpha);
    for (auto &&cmd : drawQueue_) {
        vao_->bind();
        shader_->bind();

        shader_->vertexStage()->setUniform("u_model_mat", cmd.modelMatrix);
        shader_->vertexStage()->setUniform("u_color", cmd.modelColor);

        if (cmd.drawMode == EDrawMode::Wireframe) {
            GL::manager->polygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else {
            GL::manager->polygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        if (cmd.cullMode == ECullMode::CullFrontFace) {
            GL::manager->cullFront();
        } else if (cmd.cullMode == ECullMode::CullBackFace) {
            GL::manager->cullBack();
        } else {
            GL::manager->disable(GL::Capability::CullFace);
        }

        glDrawElementsBaseVertex(GL_TRIANGLES, cmd.batch->count(), GL_UNSIGNED_INT, cmd.batch->indexOffset(), cmd.batch->baseVertex());
    }
    GL::manager->polygonMode(GL_FRONT_AND_BACK, GL_FILL);
    drawQueue_.clear();
    GL::popDebugGroup();
}
#endif  // JPH_DEBUG_RENDERER

}  // namespace PH