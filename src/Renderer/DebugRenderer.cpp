#include "DebugRenderer.h"

#include "../Camera.h"
#include "../GL/Framebuffer.h"
#include "../GL/Geometry.h"
#include "../GL/Shader.h"
#include "../GL/StateManager.h"
#include "../GL/Texture.h"
#include "../Game.h"

DebugRenderer::DebugRenderer() {
    gl::Buffer *vbo = new gl::Buffer();
    vbo->setDebugLabel("debug_renderer/vbo");
    glm::vec2 quad_verts[] = {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}};
    vbo->allocate(&quad_verts, sizeof(quad_verts), 0);

    quad = new gl::VertexArray();
    quad->setDebugLabel("debug_renderer/vao");
    quad->layout(0, 0, 2, GL_FLOAT, false, 0);
    quad->bindBuffer(0, *vbo, 0, 2 * 4);
    quad->own(vbo);

    shader = new gl::ShaderPipeline({
        new gl::ShaderProgram("assets/shaders/quad_uv.vert"),
        new gl::ShaderProgram("assets/shaders/framebuffer_debug.frag"),
    });
    shader->setDebugLabel("debug_renderer/shader");

    sampler = new gl::Sampler();
    sampler->setDebugLabel("debug_renderer/sampler");
    sampler->filterMode(GL_NEAREST, GL_NEAREST);
    sampler->wrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
}

DebugRenderer::~DebugRenderer() {
    delete shader;
    delete quad;
    delete sampler;
}

void DebugRenderer::render(Game &game) {
    gl::pushDebugGroup("DebugRenderer::render");

    gl::Framebuffer &hdr_fbo = game.hdrFramebuffer();
    gl::Texture &normals_attachment = *hdr_fbo.getTexture(1);
    gl::Texture &depth_attachment = *hdr_fbo.getTexture(GL_DEPTH_ATTACHMENT);
    Camera &camera = *game.camera;

    quad->bind();
    shader->bind();
    shader->fragmentStage()->setUniform("u_inverse_projection_mat", glm::inverse(camera.projectionMatrix()));
    sampler->bind(0);
    sampler->bind(1);

    normals_attachment.bind(0);
    depth_attachment.bind(1);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    gl::popDebugGroup();
}