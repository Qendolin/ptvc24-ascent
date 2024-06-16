#include "OrbitCam.h"

#include <glm/glm.hpp>

#include "../Camera.h"
#include "../Debug/Settings.h"
#include "../Game.h"
#include "../Input.h"
#include "../Settings.h"

void OrbitCam::update(Game& game, float time_delta) {
    auto& settings = game.debugSettings.promo;
    glm::vec3 direction = glm::vec3{
        glm::sin(glm::radians(settings.orbitAzimuth)) * glm::cos(glm::radians(settings.orbitElevation)),
        glm::sin(glm::radians(settings.orbitElevation)),
        glm::cos(glm::radians(settings.orbitAzimuth)) * glm::cos(glm::radians(settings.orbitElevation)),
    };
    game.camera->position = (direction * settings.orbitDistance) + settings.orbitOrigin;
    game.camera->angles.y = glm::radians(settings.orbitAzimuth);
    game.camera->angles.x = glm::radians(settings.orbitPitch);
    game.camera->setFov(glm::radians(settings.orbitFov));

    settings.orbitAzimuth += settings.orbitSpeed * time_delta;
    settings.orbitAzimuth = std::fmod(settings.orbitAzimuth + 360, 360.0f);
}