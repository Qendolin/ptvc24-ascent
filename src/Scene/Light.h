#pragma once

#include <glm/glm.hpp>

struct OrthoLight {
    // horizontal angle in degrees
    float azimuth;
    // vertical angle in degrees
    float elevation;

    glm::vec3 color;
    float brightness;

    glm::vec3 radiance() {
        return color * brightness;
    }

    glm::vec3 direction() {
        float az = glm::radians(azimuth);
        float el = glm::radians(elevation);
        return glm::vec3{
            glm::sin(az) * glm::cos(el),
            glm::sin(el),
            glm::cos(az) * glm::cos(el),
        };
    };
};