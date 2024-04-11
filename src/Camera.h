#pragma once

#include <glm/glm.hpp>
//#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
   private:
    // vertical field of view, in radians
    float fov_;
    glm::vec2 viewportSize_;
    float nearPlane_;

    glm::mat4 viewMatrix_;
    glm::mat4 projectionMatrix_;

    // Recalculate the projection matrix
    void updateProjectionMatrix_();

   public:
    glm::vec3 position;
    // pitch, yaw, roll
    glm::vec3 angles;

    /**
     * @param fov vertical field of view, in radians
     * @param viewport_size size of the viewport area, in pixels
     * @param near_plane distance of the near plane
     * @param position position of the camera
     * @param angles orientation of the camera
     */
    Camera(float fov, glm::vec2 viewport_size, float near_plane, glm::vec3 position, glm::vec3 angles);

    // Recalculate the view matrix
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
     */
    void setNearPlane(float near_plane) {
        nearPlane_ = near_plane;
        updateProjectionMatrix_();
    }

    /**
     * @param fov vertical field of view, in radians
     */
    void setFov(float fov) {
        fov_ = fov;
        updateProjectionMatrix_();
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

    glm::mat3 getMoveFrontVec(){
        glm::vec3 upVec = glm::vec3(0,1,0);
        glm::vec3 frontVec;
        frontVec.x = cos(glm::radians(angles.y)) * cos(glm::radians(angles.x));
        frontVec.y = cos(glm::radians(angles.x));
        frontVec.z = sin(glm::radians(angles.y)) * cos(glm::radians(angles.x));
        frontVec = glm::normalize(frontVec);
        glm::mat4 lookAtTrans = glm::lookAt(position, position + frontVec, upVec);
        glm::mat3 rotationMatrix = glm::mat3(lookAtTrans);
        return rotationMatrix;
    }
};
