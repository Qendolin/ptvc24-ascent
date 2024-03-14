#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <format>
#include <functional>
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
#include "Physics.h"
#include "Setup.h"
#include "Utils.h"

struct Arguments {
    bool enableCompatibilityProfile;
    bool disableGlDebug;
} arguments = {};

void run() {
    GLFWwindow *win = glfwGetCurrentContext();

    // =========
    // Preperation & Asset loading
    // =========

    GL::Buffer *vbo = new GL::Buffer();
    glm::vec2 quad_verts[] = {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}};
    vbo->allocate(&quad_verts, sizeof(quad_verts), 0);

    GL::VertexArray *quad = new GL::VertexArray();
    quad->layout(0, 0, 2, GL_FLOAT, false, 0);
    quad->bindBuffer(0, *vbo, 0, 2 * 4);

    std::vector<std::function<void()>> reload_callbacks;

    GL::ShaderPipeline *sky_shader = nullptr;
    GL::ShaderPipeline *test_shader = nullptr;

    DirectBuffer *dd = nullptr;

    // defer loading assets
    reload_callbacks.push_back([&sky_shader, &dd, &test_shader]() {
        if (sky_shader) sky_shader->destroy();
        sky_shader = new GL::ShaderPipeline(
            {new GL::ShaderProgram("assets/shaders/sky.vert"),
             new GL::ShaderProgram("assets/shaders/sky.frag")});

        if (dd) dd->destroy();
        auto dd_shader = new GL::ShaderPipeline(
            {new GL::ShaderProgram("assets/shaders/direct.vert"),
             new GL::ShaderProgram("assets/shaders/direct.frag")});
        dd = new DirectBuffer(dd_shader);

        if (test_shader) test_shader->destroy();
        test_shader = new GL::ShaderPipeline(
            {new GL::ShaderProgram("assets/shaders/test.vert"),
             new GL::ShaderProgram("assets/shaders/test.frag")});
    });

    int32_t viewport_dimensions[4];
    glGetIntegerv(GL_VIEWPORT, &viewport_dimensions[0]);
    glm::vec2 viewport_size = glm::vec2(viewport_dimensions[2], viewport_dimensions[3]);

    Input *input = Input::init(win);
    Camera *camera = new Camera(glm::radians(90.), viewport_size, 0.1, 100., glm::vec3{}, glm::vec3{});
    bool mouse_captured = false;

    auto instances = loadModel("assets/models/pbr_test.glb");

    // load assets
    for (const auto &callback : reload_callbacks) {
        callback();
    }

    // =========
    // Physics
    // =========

    JPH::PhysicsSystem *physics_system;
    JPH::TempAllocator *temp_allocator;
    JPH::JobSystem *job_system;
    createPhysicsSystem(physics_system, temp_allocator, job_system);
    DebugRendererImpl *physics_debug_renderer = static_cast<DebugRendererImpl *>(JPH::DebugRenderer::sInstance);
    JPH::BodyInterface &body_interface = physics_system->GetBodyInterface();

    JPH::BodyCreationSettings sphere_settings(new JPH::SphereShape(0.5f), JPH::RVec3(0.0f, 5.0f, 0.0f), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::MOVING);
    // make it bouncy
    sphere_settings.mRestitution = 0.9;
    body_interface.CreateAndAddBody(sphere_settings, JPH::EActivation::Activate);

    JPH::BoxShapeSettings floor_shape_settings(JPH::Vec3(100.0f, 1.0f, 100.0f));
    JPH::ShapeRefC floor_shape = floor_shape_settings.Create().Get();
    JPH::BodyCreationSettings floor_settings(floor_shape, JPH::RVec3(0.0, -1.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::NON_MOVING);
    body_interface.CreateAndAddBody(floor_settings, JPH::EActivation::DontActivate);

    physics_system->OptimizeBroadPhase();

    bool physics_update_enabled = false;
    float physics_update_timer = 0;
    const float physics_update_interval = 1.0f / 60.0f;

    // =========
    // Main loop
    // =========

    LOG("Entering main loop");
    while (!glfwWindowShouldClose(win)) {
        input->update();

        if (physics_update_enabled) {
            physics_update_timer += input->timeDelta();

            if (physics_update_timer > physics_update_interval) {
                physics_update_timer -= physics_update_interval;
                physics_system->Update(physics_update_interval, 1, temp_allocator, job_system);
            }
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Capture mouse
        if (input->isMousePress(GLFW_MOUSE_BUTTON_LEFT) && !mouse_captured) {
            mouse_captured = true;
            glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        // Release mouse
        if (input->isKeyPress(GLFW_KEY_ESCAPE) && mouse_captured) {
            mouse_captured = false;
            glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        // Reload assets
        if (input->isKeyPress(GLFW_KEY_F5)) {
            LOG("Reloading assets");
            for (const auto &callback : reload_callbacks) {
                callback();
            }
        }
        // Un- / Pause physics
        if (input->isKeyPress(GLFW_KEY_P)) {
            LOG("Toggle phyics update");
            physics_update_enabled = !physics_update_enabled;
        }

        // Camera movement
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
            camera->updateViewMatrix();
        }

        // Draw the loaded instances of the GLTF scene
        GL::manager->setEnabled({GL::Capability::DepthTest});
        GL::manager->depthMask(true);
        GL::manager->depthFunc(GL::DepthFunc::Less);
        test_shader->bind();
        test_shader->vertexStage()->setUniform("u_view_projection_mat", camera->viewProjectionMatrix());
        test_shader->fragmentStage()->setUniform("u_camera_pos", camera->position);

        for (auto &i : instances) {
            i.mesh.vao->bind();
            test_shader->vertexStage()->setUniform("u_model_mat", i.transform);
            for (auto &s : i.mesh.sections) {
                test_shader->fragmentStage()->setUniform("u_albedo_fac", s.material.albedoFactor);
                test_shader->fragmentStage()->setUniform("u_metallic_roughness_fac", s.material.metallicRoughnessFactor);
                if (s.material.albedo != nullptr)
                    s.material.albedo->bind(0);
                if (s.material.occlusionMetallicRoughness != nullptr)
                    s.material.occlusionMetallicRoughness->bind(1);
                if (s.material.normal != nullptr)
                    s.material.normal->bind(2);
                glDrawElementsBaseVertex(GL_TRIANGLES, s.length, GL_UNSIGNED_SHORT, 0, s.base);
            }
        }

        physics_debug_renderer->setViewProjectionMatrix(camera->viewProjectionMatrix());
        physics_system->DrawBodies({}, JPH::DebugRenderer::sInstance);

        // Draw debug
        dd->draw(camera->viewProjectionMatrix(), camera->position);

        GL::manager->setEnabled({GL::Capability::DepthTest});
        GL::manager->depthFunc(GL::DepthFunc::LessOrEqual);
        // Draw sky
        quad->bind();
        sky_shader->bind();

        sky_shader->fragmentStage()->setUniform("u_view_mat", camera->viewMatrix());
        sky_shader->fragmentStage()->setUniform("u_projection_mat", camera->projectionMatrix());

        // The sky is rendered using a single, full-screen quad
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        // Finish the frame
        glfwSwapBuffers(win);
    }
}

int main(int argc, char **argv) {
    LOG("Build from " << __DATE__ << " " << __TIME__);

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
