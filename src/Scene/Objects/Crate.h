#pragma once

#include "../Entity.h"

class CrateEntity : public scene::NodeEntity {
   private:
    scene::NodeRef colliderRef;
    scene::NodeRef meshRef;
    glm::vec3 posLerpStart;
    glm::vec3 posLerpEnd;
    glm::quat rotLerpStart;
    glm::quat rotLerpEnd;

   public:
    CrateEntity(scene::SceneRef scene, scene::NodeRef node) : scene::NodeEntity(scene, node) {
    }

    virtual ~CrateEntity(){};

    void init() override;

    void update(float time_delta) override;

    void prePhysicsUpdate() override;

    void postPhysicsUpdate() override;
};