#include "Camera.h"

#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp>

Camera::Camera(float fov, glm::vec2 viewport_size, float near_plane, float far_plane, glm::vec3 position, glm::vec3 angles) {
    this->fov_ = fov;
    this->viewportSize_ = viewport_size;
    this->nearPlane_ = near_plane;
    this->farPlane_ = far_plane;
    this->position = position;
    this->angles = angles;

    updateProjectionMatrix_();
    updateViewMatrix();
}

void Camera::updateProjectionMatrix_() {
    float aspect = viewportSize_.x / viewportSize_.y;
    projectionMatrix_ = glm::perspective(fov_, aspect, nearPlane_, farPlane_);
}

void Camera::updateViewMatrix() {
    viewMatrix_ = glm::mat4(1.0f);
    viewMatrix_ = glm::translate(viewMatrix_, position);
    viewMatrix_ = glm::rotate(viewMatrix_, angles.z, {0, 0, 1});
    viewMatrix_ = glm::rotate(viewMatrix_, angles.y, {0, 1, 0});
    viewMatrix_ = glm::rotate(viewMatrix_, angles.x, {1, 0, 0});
    viewMatrix_ = glm::inverse(viewMatrix_);
}
