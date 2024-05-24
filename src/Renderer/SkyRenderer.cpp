#include "SkyRenderer.h"

#include "../Camera.h"
#include "../GL/Geometry.h"
#include "../GL/Shader.h"
#include "../GL/StateManager.h"
#include "../GL/Texture.h"
#include "../Loader/Environment.h"

SkyRenderer::SkyRenderer(std::shared_ptr<loader::IblEnv> environment) {
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

    sampler = new gl::Sampler();
    sampler->setDebugLabel("sky_renderer/sampler");
    sampler->wrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    sampler->filterMode(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

    cubemap = new gl::Texture(GL_TEXTURE_CUBE_MAP);
    cubemap->setDebugLabel("sky_renderer/cubemap");
    cubemap->allocate(1, GL_RGB16F, environment->baseSize, environment->baseSize);
    cubemap->load(0, environment->baseSize, environment->baseSize, 6, GL_RGB, GL_FLOAT, environment->all().data());
}

SkyRenderer::~SkyRenderer() {
    delete shader;
    delete cube;
    delete sampler;
    delete cubemap;
}

void SkyRenderer::render(Camera &camera) {
    gl::pushDebugGroup("SkyRenderer::render");

    gl::manager->setEnabled({gl::Capability::DepthTest, gl::Capability::DepthClamp});
    gl::manager->depthFunc(gl::DepthFunc::GreaterOrEqual);
    gl::manager->depthMask(false);

    cube->bind();
    shader->bind();

    sampler->bind(0);
    cubemap->bind(0);

    shader->vertexStage()->setUniform("u_rotation_projection_mat", camera.projectionMatrix() * glm::mat4(glm::mat3(camera.viewMatrix())));

    // The sky is rendered using a single, full-screen quad
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 14);
    gl::popDebugGroup();
}