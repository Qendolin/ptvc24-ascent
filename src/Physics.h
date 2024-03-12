#pragma once

#include <Jolt/Jolt.h>

// Jolt includes
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Geometry/Triangle.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Renderer/DebugRenderer.h>

#include <cstdarg>
#include <iostream>
#include <thread>

class Physics {
   private:
   public:
    Physics(/* args */);
    ~Physics();
};

Physics::Physics(/* args */) {
}

Physics::~Physics() {
}

// Callback for traces, connect this to your own trace function if you have one
static void traceImpl(const char *fmt, ...) {
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
static bool assertFailedImpl(const char *expression, const char *message, const char *file, uint32_t line) {
    std::cout << file << ":" << line << ": (" << expression << ") " << (message != nullptr ? message : "") << std::endl;

    // Breakpoint
    return true;
};
#endif  // JPH_ENABLE_ASSERTS

// Layer that objects can be in, determines which other objects it can collide with
// Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can have more
// layers if you want. E.g. you could have a layer for high detail collision (which is not used by the physics simulation
// but only if you do collision testing).
namespace Layers {
static constexpr JPH::ObjectLayer NON_MOVING = 0;
static constexpr JPH::ObjectLayer MOVING = 1;
static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
};  // namespace Layers

/// Class that determines if two object layers can collide
class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
   public:
    virtual bool ShouldCollide(JPH::ObjectLayer object_a, JPH::ObjectLayer object_b) const override {
        switch (object_a) {
            case Layers::NON_MOVING:
                return object_b == Layers::MOVING;  // Non moving only collides with moving
            case Layers::MOVING:
                return true;  // Moving collides with everything
            default:
                JPH_ASSERT(false);
                return false;
        }
    }
};

// Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
// a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
// You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
// many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
// your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.
namespace BroadPhaseLayers {
static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
static constexpr JPH::BroadPhaseLayer MOVING(1);
static constexpr uint32_t NUM_LAYERS(2);
};  // namespace BroadPhaseLayers

// BroadPhaseLayerInterface implementation
// This defines a mapping between object and broadphase layers.
class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
   public:
    BPLayerInterfaceImpl() {
        // Create a mapping table from object to broad phase layer
        mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
    }

    virtual uint32_t GetNumBroadPhaseLayers() const override {
        return BroadPhaseLayers::NUM_LAYERS;
    }

    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override {
        JPH_ASSERT(layer < Layers::NUM_LAYERS);
        return mObjectToBroadPhase[layer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char *GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override {
        switch ((JPH::BroadPhaseLayer::Type)layer) {
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:
                return "NON_MOVING";
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:
                return "MOVING";
            default:
                JPH_ASSERT(false);
                return "INVALID";
        }
    }
#endif  // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

   private:
    JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

/// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter {
   public:
    virtual bool ShouldCollide(JPH::ObjectLayer layer_a, JPH::BroadPhaseLayer layer_b) const override {
        switch (layer_a) {
            case Layers::NON_MOVING:
                return layer_b == BroadPhaseLayers::MOVING;
            case Layers::MOVING:
                return true;
            default:
                JPH_ASSERT(false);
                return false;
        }
    }
};

#ifdef JPH_DEBUG_RENDERER

class BatchImpl : public JPH::RefTargetVirtual, public JPH::RefTarget<BatchImpl> {
   private:
    size_t index_ = 0;
    size_t vertex_ = 0;
    size_t count_ = 0;

   public:
    BatchImpl(size_t index, size_t vertex, size_t count)
        : index_(index), vertex_(vertex), count_(count) {
    }

    size_t count() const {
        return count_;
    }

    size_t baseVertex() const {
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

class DebugRendererImpl final : public JPH::DebugRenderer {
   private:
    GL::ShaderPipeline *shader_;
    GL::VertexArray *vao_;
    GL::Buffer *vbo_;
    GL::Buffer *ebo_;
    size_t vboWriteOffset_ = 0;
    size_t eboWriteOffset_ = 0;
    glm::mat4 viewProjectionMatrix_ = {};

   public:
    DebugRendererImpl() {
        vao_ = new GL::VertexArray();
        // Position
        vao_->layout(0, 0, 3, GL_FLOAT, false, 0);
        // Normal
        vao_->layout(0, 1, 3, GL_FLOAT, false, 3 * sizeof(float));
        // UV
        vao_->layout(0, 2, 2, GL_FLOAT, false, 6 * sizeof(float));
        // Color
        vao_->layout(0, 3, 4, GL_UNSIGNED_BYTE, true, 8 * sizeof(float));

        vbo_ = new GL::Buffer();
        vbo_->allocateEmptyMutable(sizeof(Vertex) * 8192, GL_DYNAMIC_DRAW);
        ebo_ = new GL::Buffer();
        ebo_->allocateEmptyMutable(sizeof(uint32_t) * 8192, GL_DYNAMIC_DRAW);

        vao_->bindBuffer(0, *vbo_, 0, sizeof(Vertex));
        vao_->bindElementBuffer(*ebo_);

        shader_ = new GL::ShaderPipeline({new GL::ShaderProgram("assets/shaders/physics_debug.vert"),
                                          new GL::ShaderProgram("assets/shaders/physics_debug.frag")});

        DebugRenderer::Initialize();
    }

    virtual void DrawLine(JPH::RVec3Arg from, JPH::RVec3Arg to,
                          JPH::ColorArg color) override {
        // TODO: implement
    }

    virtual void
    DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3,
                 JPH::ColorArg inColor,
                 ECastShadow inCastShadow = ECastShadow::Off) override {
        // TODO: implement
    }

    virtual Batch CreateTriangleBatch(const Triangle *triangles,
                                      int triangle_count) override {
        // TODO: implement
        return nullptr;
    }

    virtual Batch CreateTriangleBatch(const Vertex *vertices, int vertex_count,
                                      const uint32_t *indices,
                                      int index_count) override {
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

    virtual void DrawGeometry(JPH::RMat44Arg model_matrix,
                              const JPH::AABox &world_space_bounds,
                              float lod_scale_sq, JPH::ColorArg model_color,
                              const GeometryRef &geometry, ECullMode cull_mode,
                              ECastShadow cast_shadow,
                              EDrawMode draw_mode) override {
        Batch batch = geometry->mLODs[0].mTriangleBatch;
        if (batch.GetPtr() == nullptr) return;

        BatchImpl *batch_impl = static_cast<BatchImpl *>(batch.GetPtr());
        vao_->bind();
        shader_->bind();

        const glm::mat4 &glm_model_matrix = reinterpret_cast<const glm::mat4 &>(model_matrix);
        shader_->vertexStage()->setUniform("u_model_mat", glm_model_matrix);
        shader_->vertexStage()->setUniform("u_view_projection_mat", viewProjectionMatrix_);
        JPH::Vec4 model_color_vec = model_color.ToVec4();
        glm::vec4 *glm_color_vec = reinterpret_cast<glm::vec4 *>(&model_color_vec);
        shader_->vertexStage()->setUniform("u_color", *glm_color_vec);

        GL::manager->setEnabled({GL::Capability::DepthTest, GL::Capability::Blend, GL::Capability::CullFace});
        GL::manager->depthFunc(GL::DepthFunc::Less);
        GL::manager->blendEquation(GL::BlendEquation::FuncAdd);
        GL::manager->blendFunc(GL::BlendFactor::SrcAlpha, GL::BlendFactor::OneMinusSrcAlpha);

        if (draw_mode == EDrawMode::Wireframe) {
            GL::manager->polygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else {
            GL::manager->polygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        if (cull_mode == ECullMode::CullFrontFace) {
            GL::manager->cullFront();
        } else if (cull_mode == ECullMode::CullBackFace) {
            GL::manager->cullBack();
        } else {
            GL::manager->disable(GL::Capability::CullFace);
        }

        glDrawElementsBaseVertex(GL_TRIANGLES, batch_impl->count(), GL_UNSIGNED_INT, batch_impl->indexOffset(), batch_impl->baseVertex());
    }

    virtual void DrawText3D(JPH::RVec3Arg position,
                            const JPH::string_view &inString,
                            JPH::ColorArg color, float height) override {
        // Not implemented
    }

    void setViewProjectionMatrix(glm::mat4 matrix) {
        viewProjectionMatrix_ = matrix;
    }
};
#endif  // JPH_DEBUG_RENDERER

void createPhysicsSystem(JPH::PhysicsSystem *&physics_system, JPH::TempAllocator *&temp_allocator, JPH::JobSystem *&job_system) {
    // Register allocation hook. In this example we'll just let Jolt use malloc / free but you can override these if you want (see Memory.h).
    // This needs to be done before any other Jolt function is called.
    JPH::RegisterDefaultAllocator();

    // Install trace and assert callbacks
    JPH::Trace = traceImpl;
    JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = assertFailedImpl;)

    // Create a factory, this class is responsible for creating instances of classes based on their name or hash and is mainly used for deserialization of saved data.
    // It is not directly used in this example but still required.
    JPH::Factory::sInstance = new JPH::Factory();

    JPH::DebugRenderer::sInstance = new DebugRendererImpl();

    // Register all physics types with the factory and install their collision handlers with the CollisionDispatch class.
    // If you have your own custom shape types you probably need to register their handlers with the CollisionDispatch before calling this function.
    // If you implement your own default material (PhysicsMaterial::sDefault) make sure to initialize it before this function or else this function will create one for you.
    JPH::RegisterTypes();

    // We need a temp allocator for temporary allocations during the physics update. We're
    // pre-allocating 10 MB to avoid having to do allocations during the physics update.
    // B.t.w. 10 MB is way too much for this example but it is a typical value you can use.
    temp_allocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);

    // The job system will execute physics jobs.
    // For such a simple simulation a single threaded implementation should suffice.
    job_system = new JPH::JobSystemSingleThreaded(JPH::cMaxPhysicsJobs);

    // This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
    const uint32_t max_bodies = 1024;

    // This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
    // body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
    // too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
    const uint32_t max_body_pairs = 1024;

    // This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
    // number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
    const uint32_t max_contact_constraints = 1024;

    // Create mapping table from object layer to broadphase layer
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
    JPH::BroadPhaseLayerInterface *broad_phase_layer_interface = new BPLayerInterfaceImpl();

    // Create class that filters object vs broadphase layers
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
    JPH::ObjectVsBroadPhaseLayerFilter *object_vs_broadphase_layer_filter = new ObjectVsBroadPhaseLayerFilterImpl();

    // Create class that filters object vs object layers
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
    JPH::ObjectLayerPairFilter *object_vs_object_layer_filter = new ObjectLayerPairFilterImpl();

    // Now we can create the actual physics system.
    physics_system = new JPH::PhysicsSystem();
    physics_system->Init(max_bodies, 0, max_body_pairs, max_contact_constraints, *broad_phase_layer_interface, *object_vs_broadphase_layer_filter, *object_vs_object_layer_filter);
}