#pragma once

#include <Jolt/Jolt.h>
#ifdef JPH_DEBUG_RENDERER
#include <Jolt/Renderer/DebugRenderer.h>
#endif

#include <cstdarg>
#include <glm/glm.hpp>

#include "../GL/Declarations.h"

namespace ph {

// Callback for traces
void traceCallback(const char *fmt, ...);

#ifdef JPH_ENABLE_ASSERTS
// Callback for asserts, connect this to your own assert handler if you have one
bool assertFailedCallback(const char *expression, const char *message, const char *file, uint32_t line);
#endif  // JPH_ENABLE_ASSERTS

#ifdef JPH_DEBUG_RENDERER
class BatchImpl : public JPH::RefTargetVirtual, public JPH::RefTarget<BatchImpl> {
   private:
    uint32_t index_ = 0;
    uint32_t vertex_ = 0;
    uint32_t count_ = 0;

   public:
    BatchImpl(uint32_t index, uint32_t vertex, uint32_t count)
        : index_(index), vertex_(vertex), count_(count) {
    }

    uint32_t count() const {
        return count_;
    }

    uint32_t baseVertex() const {
        return vertex_;
    }

    uint32_t *indexOffset() const {
        return reinterpret_cast<uint32_t *>(index_ * sizeof(uint32_t));
    }

    virtual void AddRef() override { RefTarget::AddRef(); }
    virtual void Release() override {
        if (--mRefCount == 0) delete this;
    }
};

struct DrawCommand {
    BatchImpl *batch;
    glm::mat4 modelMatrix;
    glm::vec4 modelColor;
    JPH::DebugRenderer::ECullMode cullMode;
    JPH::DebugRenderer::EDrawMode drawMode;
};

class DebugRendererImpl final : public JPH::DebugRenderer {
   private:
    gl::ShaderPipeline *shader_;
    gl::VertexArray *vao_;
    gl::Buffer *vbo_;
    gl::Buffer *ebo_;
    size_t vboWriteOffset_ = 0;
    size_t eboWriteOffset_ = 0;
    std::vector<struct DrawCommand> drawQueue_ = {};

   public:
    DebugRendererImpl();
    ~DebugRendererImpl();

    virtual void DrawLine(JPH::RVec3Arg from, JPH::RVec3Arg to, JPH::ColorArg color) override;

    virtual void
    DrawTriangle(JPH::RVec3Arg v1, JPH::RVec3Arg v2, JPH::RVec3Arg v3, JPH::ColorArg color, ECastShadow castShadow = ECastShadow::Off) override;

    virtual Batch CreateTriangleBatch(const Triangle *triangles, int triangle_count) override;

    virtual Batch CreateTriangleBatch(const Vertex *vertices, int vertex_count, const uint32_t *indices, int index_count) override;

    virtual void DrawGeometry(JPH::RMat44Arg model_matrix,
                              const JPH::AABox &world_space_bounds,
                              float lod_scale_sq, JPH::ColorArg model_color,
                              const GeometryRef &geometry, ECullMode cull_mode,
                              ECastShadow cast_shadow,
                              EDrawMode draw_mode) override;

    virtual void DrawText3D(JPH::RVec3Arg position, const JPH::string_view &string, JPH::ColorArg color, float height) override;

    void Draw(glm::mat4 view_projection_matrix);
};
#endif  // JPH_DEBUG_RENDERER

}  // namespace ph