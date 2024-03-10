#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <format>
#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/fast_trigonometry.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>
#include <memory>
#include <sstream>

#include "Camera.h"
#include "Direct.h"
#include "GL/Framebuffer.h"
#include "GL/Geometry.h"
#include "GL/Shader.h"
#include "GL/StateManager.h"
#include "GL/Texture.h"
#include "Input.h"
#include "Loader.h"
#include "Setup.h"
#include "Utils.h"

struct Arguments {
    bool enableCompatibilityProfile;
    bool disableGlDebug;
} arguments = {};

void run() {
    GLFWwindow *win = glfwGetCurrentContext();

    GL::Buffer *vbo = new GL::Buffer();
    glm::vec2 quad_verts[] = {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}};
    vbo->allocate(&quad_verts, sizeof(quad_verts), 0);

    GL::VertexArray *quad = new GL::VertexArray();
    quad->layout(0, 0, 2, GL_FLOAT, false, 0);
    quad->bindBuffer(0, *vbo, 0, 2 * 4);

    GL::ShaderPipeline *sky_shader = new GL::ShaderPipeline(
        {new GL::ShaderProgram("assets/shaders/sky.vert"),
         new GL::ShaderProgram("assets/shaders/sky.frag")});

    GL::ShaderPipeline *dd_shader = new GL::ShaderPipeline(
        {new GL::ShaderProgram("assets/shaders/direct.vert"),
         new GL::ShaderProgram("assets/shaders/direct.frag")});
    DirectBuffer *dd = new DirectBuffer(dd_shader);

    GL::ShaderPipeline *test_shader = new GL::ShaderPipeline(
        {new GL::ShaderProgram("assets/shaders/test.vert"),
         new GL::ShaderProgram("assets/shaders/test.frag")});

    double time = glfwGetTime();
    double delta = 1 / 60.0;

    int32_t viewport_dimensions[4];
    glGetIntegerv(GL_VIEWPORT, &viewport_dimensions[0]);
    glm::vec2 viewport_size = glm::vec2(viewport_dimensions[2], viewport_dimensions[3]);

    Input *input = Input::init(win);
    Camera *camera = new Camera(glm::radians(90.), viewport_size, 0.1, 100., glm::vec3{}, glm::vec3{});
    bool mouse_captured = false;

    auto instances = loadModel("assets/models/suzanne.glb");

    LOG("Entering main loop");

    while (!glfwWindowShouldClose(win)) {
        // NOTE: input update need to be called before glfwPollEvents
        input->update();
        glfwPollEvents();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (input->isMousePress(GLFW_MOUSE_BUTTON_LEFT) && !mouse_captured) {
            mouse_captured = true;
            glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        if (input->isKeyPress(GLFW_KEY_ESCAPE) && mouse_captured) {
            mouse_captured = false;
            glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }

        if (mouse_captured) {
            // yaw
            camera->angles.y -= input->mouseDelta().x * 0.003f;
            camera->angles.y = glm::wrapAngle(camera->angles.y);
            // pitch
            camera->angles.x -= input->mouseDelta().y * 0.003f;
            camera->angles.x = glm::clamp(camera->angles.x, -glm::half_pi<float>(), glm::half_pi<float>());

            glm::vec3 move_input = {
                input->isKeyDown(GLFW_KEY_D) - input->isKeyDown(GLFW_KEY_A),
                input->isKeyDown(GLFW_KEY_SPACE) - input->isKeyDown(GLFW_KEY_LEFT_CONTROL),
                input->isKeyDown(GLFW_KEY_S) - input->isKeyDown(GLFW_KEY_W)};
            glm::vec3 move = move_input * 2.5f * input->timeDelta();
            move = glm::mat3(glm::rotate(glm::mat4(1.0), camera->angles.y, {0, 1, 0})) * move;
            camera->position += move;
            camera->updateView();
        }

        GL::manager->setEnabled({GL::Capability::DepthTest});
        GL::manager->depthMask(true);
        GL::manager->depthFunc(GL::DepthFunc::Less);
        test_shader->bind();

        test_shader->vertexStage()->setUniform("u_view_projection_mat", camera->projectionMatrix * camera->viewMatrix);
        for (auto &i : instances) {
            i.mesh.vao->bind();
            test_shader->vertexStage()->setUniform("u_model_mat", i.transform);
            for (auto &s : i.mesh.sections) {
                glDrawElementsBaseVertex(GL_TRIANGLES, s.length, GL_UNSIGNED_SHORT, 0, s.base);
            }
        }

        dd->shaded();
        dd->color(1.0, 1.0, 1.0);
        dd->plane({-10, -1, -10}, {10, -1, 10}, {0, 1, 0});

        // Draw debug
        dd->draw(camera->projectionMatrix * camera->viewMatrix, camera->position);

        // Draw sky
        quad->bind();
        sky_shader->bind();

        sky_shader->fragmentStage()->setUniform("u_view_mat", camera->viewMatrix);
        sky_shader->fragmentStage()->setUniform("u_projection_mat", camera->projectionMatrix);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        // Poll events
        glfwPollEvents();
        glfwSwapBuffers(win);
        delta = glfwGetTime() - time;
        time += delta;
    }
}

int main(int argc, char **argv) {
    LOG("Parsing arguments");
    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);
        if (arg == "--enable-compatibility-profile") {
            arguments.enableCompatibilityProfile = true;
        }
        if (arg == "--disable-gl-debug") {
            arguments.disableGlDebug = true;
        }
    }

    try {
        setupOpenGL(arguments.enableCompatibilityProfile, arguments.disableGlDebug);
        run();
    } catch (const std::exception &e) {
        std::cerr << "Fatal Error: " << e.what() << std::flush;
        return EXIT_FAILURE;
    }

    std::cerr << std::flush;
    LOG("Exit");
}
