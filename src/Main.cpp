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
#include "Utils.h"

struct Arguments {
    bool enableCompatibilityProfile;
    bool disableGlDebug;
} arguments = {};

static void APIENTRY DebugCallbackDefault(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar *message,
    const GLvoid *userParam) {
    if (id == 131185 || id == 131218)
        return;  // ignore performance warnings from nvidia
    std::string error = formatDebugOutput(source, type, id, severity, message);
    std::cout << error << std::endl;
}

// Using globals isn't so pretty but it get's the job done
GLFWwindow *win;

void setup() {
    LOG("Initializing GLFW");
    if (glfwInit() != GLFW_TRUE) {
        throw std::runtime_error("GLFW init failed");
    }

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    if (arguments.enableCompatibilityProfile) {
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
        LOG("Using GL compatability profile");
    } else {
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        LOG("Using GL core profile");
    }

    LOG("Creating Window");
    win = glfwCreateWindow(1600, 900, "Ascent", nullptr, nullptr);
    if (win == nullptr) {
        throw std::runtime_error("GLFW window creation failed");
    }
    glfwMakeContextCurrent(win);

    LOG("Initializing OpenGL");

    glewExperimental = true;
    GLenum err = glewInit();

    // If GLEW wasn't initialized
    if (err != GLEW_OK) {
        throw std::runtime_error(std::format(
            "Glew init failed: {}",
            reinterpret_cast<const char *>(glewGetErrorString(err))));
    }

    LOG("Using GPU: " << glGetString(GL_RENDERER));

    GL::manager = std::make_unique<GL::StateManager>(GL::createEnvironment());

    // enable these without using the manager
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    if (glDebugMessageCallback != NULL && !arguments.disableGlDebug) {
        glDebugMessageCallback(DebugCallbackDefault, NULL);
        glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_PUSH_GROUP, GL_DONT_CARE, 0, nullptr, false);
        glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_POP_GROUP, GL_DONT_CARE, 0, nullptr, false);
    }
}

void run() {
    GL::Buffer *vbo = new GL::Buffer();
    glm::vec2 quad_verts[] = {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}};
    vbo->allocate(&quad_verts, sizeof(quad_verts), 0);

    GL::VertexArray *quad = new GL::VertexArray();
    quad->layout(0, 0, 2, GL_FLOAT, false, 0);
    quad->bindBuffer(0, *vbo, 0, 2 * 4);

    GL::ShaderProgram *vert_sh = new GL::ShaderProgram(loadFile("assets/shaders/sky.vert"), GL_VERTEX_SHADER);
    vert_sh->compile();
    GL::ShaderProgram *frag_sh = new GL::ShaderProgram(loadFile("assets/shaders/sky.frag"), GL_FRAGMENT_SHADER);
    frag_sh->compile();

    GL::ShaderPipeline *shader = new GL::ShaderPipeline();
    shader->attach(vert_sh);
    shader->attach(frag_sh);

    GL::ShaderProgram *dd_vert_sh = new GL::ShaderProgram(loadFile("assets/shaders/direct.vert"), GL_VERTEX_SHADER);
    dd_vert_sh->compile();
    GL::ShaderProgram *dd_frag_sh = new GL::ShaderProgram(loadFile("assets/shaders/direct.frag"), GL_FRAGMENT_SHADER);
    dd_frag_sh->compile();

    GL::ShaderPipeline *dd_shader = new GL::ShaderPipeline();
    dd_shader->attach(dd_vert_sh);
    dd_shader->attach(dd_frag_sh);
    DirectBuffer *dd = new DirectBuffer(dd_shader);

    double time = glfwGetTime();
    double delta = 1 / 60.0;

    int32_t viewport_dimensions[4];
    glGetIntegerv(GL_VIEWPORT, &viewport_dimensions[0]);
    glm::vec2 viewport_size = glm::vec2(viewport_dimensions[2], viewport_dimensions[3]);

    Input *input = Input::init(win);
    Camera *camera = new Camera(glm::radians(90.), viewport_size, 0.1, 100., glm::vec3{}, glm::vec3{});
    bool mouse_captured = false;

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

        quad->bind();
        shader->bind();

        frag_sh->setUniform("u_view_mat", camera->viewMatrix);
        frag_sh->setUniform("u_projection_mat", camera->projectionMatrix);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        dd->shaded();
        dd->color(1.0, 1.0, 1.0);
        dd->plane({-10, -1, -10}, {10, -1, 10}, {0, 1, 0});
        dd->draw(camera->projectionMatrix * camera->viewMatrix, camera->position);

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
        setup();
        run();
    } catch (const std::exception &e) {
        std::cerr << "Fatal Error: " << e.what() << std::flush;
        return EXIT_FAILURE;
    }

    std::cerr << std::flush;
    LOG("Exit");
}
