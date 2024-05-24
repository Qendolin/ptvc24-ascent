#include "FreeCam.h"

#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/fast_trigonometry.hpp>
#include <glm/gtx/transform.hpp>

#include "../Camera.h"
#include "../Game.h"
#include "../Input.h"
#include "../Util/Log.h"
#include "../Window.h"

void FreeCamEntity::update(float time_delta) {
    Game &game = Game::get();
    Input &input = *game.input;
    Settings settings = game.settings.get();

    camera.setFov(glm::radians(settings.fov));

    // yaw
    camera.angles.y -= input.mouseDelta().x * glm::radians(settings.lookSensitivity);
    camera.angles.y = glm::wrapAngle(camera.angles.y);

    // pitch
    camera.angles.x -= input.mouseDelta().y * glm::radians(settings.lookSensitivity);
    camera.angles.x = glm::clamp(camera.angles.x, -glm::half_pi<float>(), glm::half_pi<float>());

    // Calculate movement input. Use the trick that in c++ we can substract booleans
    glm::vec3 move_input = {
        input.isKeyDown(GLFW_KEY_D) - input.isKeyDown(GLFW_KEY_A),
        input.isKeyDown(GLFW_KEY_SPACE) - input.isKeyDown(GLFW_KEY_LEFT_CONTROL),
        input.isKeyDown(GLFW_KEY_S) - input.isKeyDown(GLFW_KEY_W)};

    float speed_change = input.scrollDelta().y * speed_ / 10;
    speed_ = std::clamp(speed_ + speed_change, MIN_SPEED, MAX_SPEED);
    glm::vec3 velocity = move_input * speed_;

    // Rotate the velocity vector towards look direction
    velocity = glm::mat3(glm::rotate(glm::mat4(1.0), camera.angles.y, {0, 1, 0})) * velocity;

    camera.position += velocity * time_delta;
    camera.updateViewMatrix();
}
