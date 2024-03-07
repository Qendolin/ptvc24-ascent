#pragma once

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

class Camera {
   private:
   public:
    float fovRad;
    glm::vec2 viewportSize;
    float nearPlane;
    float farPlane;
    glm::vec3 position;
    // pitch, yaw, roll
    glm::vec3 angles;
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;

    Camera(float fovRad, glm::vec2 viewportSize, float nearPlane, float farPlane, glm::vec3 position, glm::vec3 angles);

    void updateProjection();
    void updateView();
};
