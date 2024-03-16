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
#include <Jolt/Physics/Character/Character.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Renderer/DebugRenderer.h>

#include <cstdarg>
#include <functional>
#include <glm/glm.hpp>
#include <iostream>
#include <thread>

#include "Debug.h"
#include "Layers.h"

namespace PH {

inline JPH::Vec3 convert(glm::vec3 v) {
    return {v.x, v.y, v.z};
}

inline glm::vec3 convert(JPH::Vec3 v) {
    return {v.GetX(), v.GetY(), v.GetZ()};
}

inline JPH::Vec4 convert(glm::vec4 v) {
    return {v.x, v.y, v.z, v.w};
}

inline glm::vec4 convert(JPH::Vec4 v) {
    return {v.GetX(), v.GetY(), v.GetZ(), v.GetW()};
}

typedef struct SensorContact {
    JPH::BodyID sensor;
    JPH::BodyID other;
    bool persistent;
} SensorContact;

// The contact listener records all sensor contacts
// It also provides a way to register sensor contact callbacks
class SensorContactListener : public JPH::ContactListener {
   private:
    std::vector<SensorContact> recordedSensorContacts_ = {};
    std::unordered_map<JPH::BodyID, std::function<void(SensorContact)>> registeredSensors_ = {};

   public:
    virtual void OnContactAdded(const JPH::Body &body_a, const JPH::Body &body_b, const JPH::ContactManifold &manifold, JPH::ContactSettings &settings) override;

    virtual void OnContactPersisted(const JPH::Body &body_a, const JPH::Body &body_b, const JPH::ContactManifold &manifold, JPH::ContactSettings &settings) override;

    void RecordSensorContact(const JPH::BodyID sensor, const JPH::BodyID other, bool persistent);

    void DispatchCallbacks();

    void RegisterCallback(JPH::BodyID sensor_id, std::function<void(SensorContact)> callback);

    void UnrgisterCallback(JPH::BodyID sensor_id);
};

struct PhysicsSetupConfig {
    // This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
    uint32_t maxBodies = 1024;

    // This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
    // body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
    // too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
    uint32_t maxBodyPairs = 1024;

    // This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
    // number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
    uint32_t maxContactConstraints = 1024;
};

class Physics {
   private:
    inline static const float UPDATE_INTERVAL = 1.0 / 60.0;

    float updateTimer_ = 0;
    bool updateEnabled_ = true;

    JPH::Factory *factory_;
    JPH::TempAllocator *tempAllocator_;
    JPH::JobSystem *jobSystem_;

#ifdef JPH_DEBUG_RENDERER
    DebugRendererImpl *debugRenderer_;
#endif

   public:
    JPH::PhysicsSystem *system;
    SensorContactListener *contactListener;

    Physics(PhysicsSetupConfig config);
    ~Physics();

    void update(float delta);

    // returns true when step() should be called
    bool isNextStepDue();

    // andvances the phyics simulation
    void step();

    // returns a factor [0;1] between the last tick and the next one.
    float partialTicks();

    void debugDraw(glm::mat4 view_projection_matrix);

    bool enabled() {
        return updateEnabled_;
    }

    void setEnabled(bool enabled) {
        updateEnabled_ = enabled;
    }

    JPH::BodyInterface &interface() {
        return system->GetBodyInterface();
    }
};

}  // namespace PH
