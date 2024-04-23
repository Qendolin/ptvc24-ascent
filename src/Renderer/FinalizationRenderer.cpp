#include "FinalizationRenderer.h"

#include "../Camera.h"
#include "../GL/Geometry.h"
#include "../GL/Shader.h"
#include "../GL/StateManager.h"
#include "../GL/Texture.h"
#include "../Game.h"

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

    fboSampler = new gl::Sampler();
    fboSampler->setDebugLabel("finalization_renderer/fbo_sampler");
    fboSampler->wrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0);
    fboSampler->filterMode(GL_NEAREST, GL_NEAREST);

    bloomSampler = new gl::Sampler();
    bloomSampler->setDebugLabel("finalization_renderer/bloom_sampler");
    bloomSampler->wrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0);
    bloomSampler->filterMode(GL_LINEAR, GL_LINEAR);
}

FinalizationRenderer::~FinalizationRenderer() {
    delete shader;
    delete quad;
    delete fboSampler;
    delete bloomSampler;
}

void FinalizationRenderer::render(gl::Texture *hrd_color, gl::Texture *depth, gl::Texture *bloom) {
    gl::pushDebugGroup("FinalizationRenderer::render");

    gl::manager->setEnabled({gl::Capability::DepthTest});
    gl::manager->depthMask(true);
    gl::manager->depthFunc(gl::DepthFunc::Always);

    quad->bind();
    shader->bind();

    fboSampler->bind(0);
    fboSampler->bind(1);
    bloomSampler->bind(2);

    hrd_color->bind(0);
    depth->bind(1);
    bloom->bind(2);

    shader->fragmentStage()->setUniform("u_bloom_fac", Game::get().debugSettings.rendering.bloom.factor);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    gl::popDebugGroup();
}