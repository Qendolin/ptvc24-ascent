#include "Camera.h"

#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp>

Camera::Camera(float fov, float near_plane, glm::vec3 position, glm::vec3 angles) {
    this->fov_ = fov;
    this->nearPlane_ = near_plane;
    this->position = position;
    this->angles = angles;

    updateProjectionMatrix_();
    updateViewMatrix();
}

Camera::~Camera() = default;

void Camera::updateProjectionMatrix_() {
    float a = viewportSize_.x / viewportSize_.y;
    float f = 1.0f / std::tan(fov_ / 2.0f);
    // This is a reversed projection matrix with an infite far plane.
    projectionMatrix_ = glm::mat4(
        f / a, 0.0f, 0.0f, 0.0f,
        0.0f, f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, nearPlane_, 0.0f);
}

void Camera::updateViewMatrix() {
    viewMatrix_ = glm::mat4(1.0f);
    viewMatrix_ = glm::translate(viewMatrix_, position);
    viewMatrix_ = glm::rotate(viewMatrix_, angles.z, {0, 0, 1});
    viewMatrix_ = glm::rotate(viewMatrix_, angles.y, {0, 1, 0});
    viewMatrix_ = glm::rotate(viewMatrix_, angles.x, {1, 0, 0});
    viewMatrix_ = glm::inverse(viewMatrix_);
}
