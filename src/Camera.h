#pragma once

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

class Camera {
   private:
    // vertical field of view, in radians
    float fov_;
    glm::vec2 viewportSize_;
    float nearPlane_;
    float farPlane_;

    glm::mat4 viewMatrix_;
    glm::mat4 projectionMatrix_;

    void updateProjectionMatrix_();

   public:
    glm::vec3 position;
    // pitch, yaw, roll
    glm::vec3 angles;

    /**
     * @param fov vertical field of view, in radians
     * @param viewport_size size of the viewport area, in pixels
     * @param near_plane distance of the near plane
     * @param far_plane distance of the far plane
     * @param position position of the camera
     * @param angles orientation of the camera
     */
    Camera(float fov, glm::vec2 viewport_size, float near_plane, float far_plane, glm::vec3 position, glm::vec3 angles);

    void updateViewMatrix();

    /**
     * @param viewport_size size of the viewport area, in pixels
     */
    void setViewportSize(glm::vec2 viewport_size) {
        viewportSize_ = viewport_size;
        updateProjectionMatrix_();
    }

    /**
     * @param near_plane distance of the near plane
     * @param far_plane distance of the far plane
     */
    void setPlanes(float near_plane, float far_plane) {
        nearPlane_ = near_plane;
        farPlane_ = far_plane;
        updateProjectionMatrix_();
    }

    /**
     * @param fov vertical field of view, in radians
     */
    void setFov(float near_plane, float far_plane) {
        nearPlane_ = near_plane;
        farPlane_ = far_plane;
        updateProjectionMatrix_();
    }

    glm::mat4 projectionMatrix() const {
        return projectionMatrix_;
    }

    glm::mat4 viewMatrix() const {
        return viewMatrix_;
    }

    glm::mat3 rotationMatrix() const {
        return glm::mat3(viewMatrix_);
    }

    glm::mat4 viewProjectionMatrix() const {
        return projectionMatrix_ * viewMatrix_;
    }
};
