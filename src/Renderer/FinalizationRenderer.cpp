#include "FinalizationRenderer.h"

#include "../Camera.h"
#include "../GL/Geometry.h"
#include "../GL/Shader.h"
#include "../GL/StateManager.h"
#include "../GL/Texture.h"

FinalizationRenderer::FinalizationRenderer() {
    shader = new gl::ShaderPipeline(
        {new gl::ShaderProgram("assets/shaders/quad_uv.vert"),
         new gl::ShaderProgram("assets/shaders/finalization.frag")});
    shader->setDebugLabel("finalization_renderer/shader");

    gl::Buffer *vbo = new gl::Buffer();
    vbo->setDebugLabel("finalization_renderer/vbo");
    glm::vec2 quad_verts[] = {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}};
    vbo->allocate(&quad_verts, sizeof(quad_verts), 0);

    quad = new gl::VertexArray();
    quad->setDebugLabel("finalization_renderer/vao");
    quad->layout(0, 0, 2, GL_FLOAT, false, 0);
    quad->bindBuffer(0, *vbo, 0, 2 * 4);
    quad->own(vbo);

    sampler = new gl::Sampler();
    sampler->setDebugLabel("finalization_renderer/sampler");
    sampler->wrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0);
    sampler->filterMode(GL_NEAREST, GL_NEAREST);
}

FinalizationRenderer::~FinalizationRenderer() {
    delete shader;
    delete quad;
    delete sampler;
}

void FinalizationRenderer::render(gl::Texture *hrd_color, gl::Texture *depth) {
    gl::pushDebugGroup("FinalizationRenderer::render");

    gl::manager->setEnabled({gl::Capability::DepthTest});
    gl::manager->depthMask(true);
    gl::manager->depthFunc(gl::DepthFunc::Always);

    quad->bind();
    shader->bind();

    sampler->bind(0);
    sampler->bind(1);

    hrd_color->bind(0);
    depth->bind(1);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    gl::popDebugGroup();
}