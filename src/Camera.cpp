#include "Camera.h"

#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp>

Camera::Camera(float fovRad, glm::vec2 viewportSize, float nearPlane, float farPlane, glm::vec3 position, glm::vec3 angles) {
    this->fovRad = fovRad;
    this->viewportSize = viewportSize;
    this->nearPlane = nearPlane;
    this->farPlane = farPlane;
    this->position = position;
    this->angles = angles;

    updateProjection();
    updateView();
}

void Camera::updateProjection() {
    float aspect = viewportSize.x / viewportSize.y;
    projectionMatrix = glm::perspective(fovRad, aspect, nearPlane, farPlane);
}

void Camera::updateView() {
    viewMatrix = glm::mat4(1.0f);
    viewMatrix = glm::translate(viewMatrix, position);
    viewMatrix = glm::rotate(viewMatrix, angles.z, {0, 0, 1});
    viewMatrix = glm::rotate(viewMatrix, angles.y, {0, 1, 0});
    viewMatrix = glm::rotate(viewMatrix, angles.x, {1, 0, 0});
    viewMatrix = glm::inverse(viewMatrix);
}
