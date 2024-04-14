#include "SkyRenderer.h"

#include "../Camera.h"
#include "../GL/Geometry.h"
#include "../GL/Shader.h"
#include "../GL/StateManager.h"

SkyRenderer::SkyRenderer() {
    shader = new gl::ShaderPipeline(
        {new gl::ShaderProgram("assets/shaders/sky.vert"),
         new gl::ShaderProgram("assets/shaders/sky.frag")});
    shader->setDebugLabel("sky_renderer/shader");

    gl::Buffer *vbo = new gl::Buffer();
    vbo->setDebugLabel("sky_renderer/vbo");
    glm::vec2 quad_verts[] = {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}};
    vbo->allocate(&quad_verts, sizeof(quad_verts), 0);

    quad = new gl::VertexArray();
    quad->setDebugLabel("sky_renderer/vao");
    quad->layout(0, 0, 2, GL_FLOAT, false, 0);
    quad->bindBuffer(0, *vbo, 0, 2 * 4);
    quad->own(vbo);
}

SkyRenderer::~SkyRenderer() {
    delete shader;
    delete quad;
}

void SkyRenderer::render(Camera &camera) {
    gl::pushDebugGroup("SkyRenderer::render");

    gl::manager->setEnabled({gl::Capability::DepthTest, gl::Capability::DepthClamp});
    gl::manager->depthFunc(gl::DepthFunc::GreaterOrEqual);

    quad->bind();
    shader->bind();

    shader->fragmentStage()->setUniform("u_view_mat", camera.viewMatrix());
    shader->fragmentStage()->setUniform("u_projection_mat", camera.projectionMatrix());

    // The sky is rendered using a single, full-screen quad
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    gl::popDebugGroup();
}