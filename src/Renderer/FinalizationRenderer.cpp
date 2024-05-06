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

void FinalizationRenderer::render(gl::Texture *hrd_color, gl::Texture *depth, gl::Texture *bloom, gl::Texture *flares, gl::Texture *glare) {
    gl::pushDebugGroup("FinalizationRenderer::render");

    gl::manager->setEnabled({});

    quad->bind();
    shader->bind();

    fboSampler->bind(0);
    fboSampler->bind(1);
    bloomSampler->bind(2);

    hrd_color->bind(0);
    depth->bind(1);
    bloom->bind(2);
    flares->bind(3);
    glare->bind(4);

    auto &settings = Game::get().debugSettings.rendering;

    shader->fragmentStage()->setUniform("u_bloom_fac", settings.bloom.factor);
    shader->fragmentStage()->setUniform("u_flares_fac", settings.lens.factor);
    shader->fragmentStage()->setUniform("u_vignette_params", glm::vec4(settings.vignette.factor, settings.vignette.inner, settings.vignette.outer, settings.vignette.sharpness));

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    gl::popDebugGroup();
}