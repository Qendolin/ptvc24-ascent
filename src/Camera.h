#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
   private:
    // vertical field of view, in radians
    float fov_;
    // aspect ratio (width / height)
    float aspect_;
    glm::vec2 viewportSize_ = {1600, 900};
    float nearPlane_;

    glm::mat4 viewMatrix_;
    glm::mat4 projectionMatrix_;

    // Recalculate the projection matrix
    void updateProjectionMatrix_();

   public:
    glm::vec3 position;
    // pitch, yaw, roll in radians
    glm::vec3 angles;

    /**
     * @param fov vertical field of view, in radians
     * @param near_plane distance of the near plane
     * @param position position of the camera
     * @param angles orientation of the camera
     */
    Camera(float fov, float near_plane, glm::vec3 position, glm::vec3 angles);
    ~Camera();

    // Recalculate the view matrix
    void updateViewMatrix();

    /**
     * @param width of the viewport area, in pixels
     * @param height of the viewport area, in pixels
     */
    void setViewport(int width, int height) {
        viewportSize_ = {width, height};
        updateProjectionMatrix_();
    }

    /**
     * @param near_plane distance of the near plane
     */
    void setNearPlane(float near_plane) {
        nearPlane_ = near_plane;
        updateProjectionMatrix_();
    }

    float nearPlane() {
        return nearPlane_;
    }

    /**
     * @param fov vertical field of view, in radians
     */
    void setFov(float fov) {
        fov_ = fov;
        updateProjectionMatrix_();
    }

    /**
     * @return the vertical fov in rad
     */
    float fov() const {
        return fov_;
    }

    /**
     * @return the frustum aspect ratio (width / height)
     */
    float aspect() const {
        return aspect_;
    }

    glm::mat4 projectionMatrix() const {
        return projectionMatrix_;
    }

    glm::mat4 viewMatrix() const {
        return viewMatrix_;
    }

    glm::mat3 rotationMatrix() const {
        return glm::transpose(glm::mat3(viewMatrix_));
    }

    glm::mat4 viewProjectionMatrix() const {
        return projectionMatrix_ * viewMatrix_;
    }
};
