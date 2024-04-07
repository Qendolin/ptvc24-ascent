#include "Physics.h"

#include <mutex>

namespace PH {

void SensorContactListener::OnContactAdded(const JPH::Body &body_a, const JPH::Body &body_b, const JPH::ContactManifold &manifold, JPH::ContactSettings &settings) {
    if (body_a.GetObjectLayer() == Layers::SENSOR) {
        RecordSensorContact(body_a.GetID(), body_b.GetID(), false);
    }
    if (body_b.GetObjectLayer() == Layers::SENSOR) {
        RecordSensorContact(body_b.GetID(), body_a.GetID(), false);
    }
}

void SensorContactListener::OnContactPersisted(const JPH::Body &body_a, const JPH::Body &body_b, const JPH::ContactManifold &manifold, JPH::ContactSettings &settings) {
    if (body_a.GetObjectLayer() == Layers::SENSOR) {
        RecordSensorContact(body_a.GetID(), body_b.GetID(), true);
    }
    if (body_b.GetObjectLayer() == Layers::SENSOR) {
        RecordSensorContact(body_b.GetID(), body_a.GetID(), true);
    }
}

void SensorContactListener::RecordSensorContact(const JPH::BodyID sensor, const JPH::BodyID other, bool persistent) {
    recordedSensorContacts_.push_back({sensor, other, persistent});
}

void SensorContactListener::DispatchCallbacks() {
    for (auto &&contact : recordedSensorContacts_) {
        if (registeredSensors_.count(contact.sensor) > 0) {
            registeredSensors_[contact.sensor](contact);
        }
    }

    recordedSensorContacts_.clear();
}

void SensorContactListener::RegisterCallback(JPH::BodyID sensor_id, std::function<void(SensorContact)> callback) {
    registeredSensors_[sensor_id] = callback;
}

void SensorContactListener::UnrgisterCallback(JPH::BodyID sensor_id) {
    registeredSensors_.erase(sensor_id);
}

std::once_flag jph_initalized;

Physics::Physics(PhysicsSetupConfig config) {
    std::call_once(jph_initalized, [] {
        // Register allocation hook. In this example we'll just let Jolt use malloc / free but you can override these if you want (see Memory.h).
        // This needs to be done before any other Jolt function is called.
        JPH::RegisterDefaultAllocator();

        // Install trace and assert callbacks
        JPH::Trace = traceCallback;
        JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = assertFailedCallback;)
    });

    if (JPH::Factory::sInstance) {
        // Unregisters all types with the factory
        JPH::UnregisterTypes();

        // Destroy the factory
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }

    // Create a factory, this class is responsible for creating instances of classes based on their name or hash and is mainly used for deserialization of saved data.
    // It is not directly used in this example but still required.
    factory_ = new JPH::Factory();
    JPH::Factory::sInstance = factory_;

    // Register all physics types with the factory and install their collision handlers with the CollisionDispatch class.
    // If you have your own custom shape types you probably need to register their handlers with the CollisionDispatch before calling this function.
    // If you implement your own default material (PhysicsMaterial::sDefault) make sure to initialize it before this function or else this function will create one for you.
    JPH::RegisterTypes();

    if (JPH::DebugRenderer::sInstance) {
        delete JPH::DebugRenderer::sInstance;
        JPH::DebugRenderer::sInstance = nullptr;
    }

#ifdef JPH_DEBUG_RENDERER
    // Create a debug renderer for displaying physics bodies and other properties
    debugRenderer_ = new DebugRendererImpl();
    JPH::DebugRenderer::sInstance = debugRenderer_;
#endif

    // We need a temp allocator for temporary allocations during the physics update. We're
    // pre-allocating 10 MB to avoid having to do allocations during the physics update.
    // B.t.w. 10 MB is way too much for this example but it is a typical value you can use.
    tempAllocator_ = new JPH::TempAllocatorImpl(10 * 1024 * 1024);

    // The job system will execute physics jobs.
    // For such a simple simulation a single threaded implementation should suffice.
    jobSystem_ = new JPH::JobSystemSingleThreaded(JPH::cMaxPhysicsJobs);

    // Create class that filters object vs object layers
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
    JPH::ObjectLayerPairFilterTable *obj_vs_obj_filter = new JPH::ObjectLayerPairFilterTable(Layers::NUM_LAYERS);
    obj_vs_obj_filter->EnableCollision(Layers::MOVING, Layers::NON_MOVING);
    obj_vs_obj_filter->EnableCollision(Layers::MOVING, Layers::MOVING);
    obj_vs_obj_filter->EnableCollision(Layers::MOVING, Layers::SENSOR);

    // Create mapping table from object layer to broadphase layer
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
    JPH::BroadPhaseLayerInterfaceTable *bp_layer_interface = new JPH::BroadPhaseLayerInterfaceTable(Layers::NUM_LAYERS, BroadPhaseLayers::NUM_LAYERS);
    bp_layer_interface->MapObjectToBroadPhaseLayer(Layers::NON_MOVING, BroadPhaseLayers::NON_MOVING);
    bp_layer_interface->MapObjectToBroadPhaseLayer(Layers::MOVING, BroadPhaseLayers::MOVING);
    bp_layer_interface->MapObjectToBroadPhaseLayer(Layers::SENSOR, BroadPhaseLayers::NON_MOVING);

    // Create class that filters object vs broadphase layers
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
    JPH::ObjectVsBroadPhaseLayerFilterTable *obj_vs_bp_filter = new JPH::ObjectVsBroadPhaseLayerFilterTable(
        *bp_layer_interface,
        BroadPhaseLayers::NUM_LAYERS,
        *obj_vs_obj_filter,
        Layers::NUM_LAYERS);

    // Now we can create the actual physics system.
    system = new JPH::PhysicsSystem();
    system->Init(config.maxBodies, 0, config.maxBodyPairs, config.maxContactConstraints, *bp_layer_interface, *obj_vs_bp_filter, *obj_vs_obj_filter);

    contactListener = new SensorContactListener();
    system->SetContactListener(contactListener);
}

Physics::~Physics() {
#ifdef JPH_DEBUG_RENDERER
    delete debugRenderer_;
#endif

    delete system;
    delete tempAllocator_;
    delete jobSystem_;
    delete contactListener;

    // Destroy the factory
    if (JPH::Factory::sInstance == factory_) {
        JPH::UnregisterTypes();
        JPH::Factory::sInstance = nullptr;
    }
    delete factory_;
}

void Physics::update(float delta) {
    if (!updateEnabled_)
        return;

    updateTimer_ += delta;
    // limit the update frequency in case of lag
    updateTimer_ = std::min(updateTimer_, 2 * UPDATE_INTERVAL);
}

bool Physics::isNextStepDue() const {
    return updateTimer_ > UPDATE_INTERVAL;
}

void Physics::step() {
    updateTimer_ = std::max(updateTimer_ - UPDATE_INTERVAL, 0.0f);
    system->Update(UPDATE_INTERVAL, 1, tempAllocator_, jobSystem_);
    contactListener->DispatchCallbacks();
}

float Physics::partialTicks() const {
    return std::clamp(updateTimer_ / UPDATE_INTERVAL, 0.0f, 1.0f);
}

void Physics::debugDraw(glm::mat4 view_projection_matrix) {
#ifdef JPH_DEBUG_RENDERER
    system->DrawBodies({}, debugRenderer_);
    debugRenderer_->Draw(view_projection_matrix);
#endif
}

}  // namespace PH