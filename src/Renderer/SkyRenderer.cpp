#include "SkyRenderer.h"

#include "../Camera.h"
#include "../GL/Geometry.h"
#include "../GL/Shader.h"
#include "../GL/StateManager.h"
#include "../GL/Texture.h"
#include "../Loader/Environment.h"

SkyRenderer::SkyRenderer() {
    shader = new gl::ShaderPipeline(
        {new gl::ShaderProgram("assets/shaders/sky.vert"),
         new gl::ShaderProgram("assets/shaders/sky.frag")});
    shader->setDebugLabel("sky_renderer/shader");

    // A single TRIANGLE_STRIP cube
    // clang-format off
    float cube_mesh[] = {
    // [position        ]
    // [  x     y     z ]
        -.5f, -.5f, -.5f,
        +.5f, -.5f, -.5f,
        -.5f, -.5f, +.5f,
        +.5f, -.5f, +.5f,
        +.5f, +.5f, +.5f,
        +.5f, -.5f, -.5f,
        +.5f, +.5f, -.5f,
        -.5f, -.5f, -.5f,
        -.5f, +.5f, -.5f,
        -.5f, -.5f, +.5f,
        -.5f, +.5f, +.5f,
        +.5f, +.5f, +.5f,
        -.5f, +.5f, -.5f,
        +.5f, +.5f, -.5f,
    };
    // clang-format on

    gl::Buffer *vbo = new gl::Buffer();
    vbo->setDebugLabel("sky_renderer/vbo");
    vbo->allocate(&cube_mesh, sizeof(cube_mesh), 0);

    cube = new gl::VertexArray();
    cube->setDebugLabel("sky_renderer/vao");
    cube->layout(0, 0, 3, GL_FLOAT, false, 0);
    cube->bindBuffer(0, *vbo, 0, 3 * sizeof(float));
    cube->own(vbo);
}

SkyRenderer::~SkyRenderer() {
    delete shader;
    delete cube;
}

void SkyRenderer::render(Camera &camera, loader::Environment &env) {
    gl::pushDebugGroup("SkyRenderer::render");

    gl::manager->setEnabled({gl::Capability::DepthTest, gl::Capability::DepthClamp});
    gl::manager->depthFunc(gl::DepthFunc::GreaterOrEqual);
    gl::manager->depthMask(false);

    cube->bind();
    shader->bind();

    env.cubemapSampler().bind(0);
    env.sky().bind(0);

    shader->vertexStage()->setUniform("u_rotation_projection_mat", camera.projectionMatrix() * glm::mat4(glm::mat3(camera.viewMatrix())));

    // The sky is rendered using a single, full-screen quad
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 14);
    gl::popDebugGroup();
}