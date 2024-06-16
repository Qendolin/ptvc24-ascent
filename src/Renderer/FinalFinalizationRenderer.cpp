#include "FinalFinalizationRenderer.h"

#include "../Camera.h"
#include "../GL/Framebuffer.h"
#include "../GL/Geometry.h"
#include "../GL/Shader.h"
#include "../GL/StateManager.h"
#include "../GL/Texture.h"
#include "../Game.h"

FinalFinalizationRenderer::FinalFinalizationRenderer() {
    shader = new gl::ShaderPipeline(
        {new gl::ShaderProgram("assets/shaders/quad_uv.vert"),
         new gl::ShaderProgram("assets/shaders/final_finalization.frag")});
    shader->setDebugLabel("final_finalization_renderer/shader");

    gl::Buffer *vbo = new gl::Buffer();
    vbo->setDebugLabel("final_finalization_renderer/vbo");
    glm::vec2 quad_verts[] = {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}};
    vbo->allocate(&quad_verts, sizeof(quad_verts), 0);

    quad = new gl::VertexArray();
    quad->setDebugLabel("final_finalization_renderer/vao");
    quad->layout(0, 0, 2, GL_FLOAT, false, 0);
    quad->bindBuffer(0, *vbo, 0, 2 * 4);
    quad->own(vbo);

    fboSampler = new gl::Sampler();
    fboSampler->setDebugLabel("final_finalization_renderer/fbo_sampler");
    fboSampler->wrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0);
    fboSampler->filterMode(GL_LINEAR, GL_LINEAR);
}

FinalFinalizationRenderer::~FinalFinalizationRenderer() {
    delete shader;
    delete quad;
    delete fboSampler;
}

void FinalFinalizationRenderer::render(gl::Texture *sdr_color) {
    gl::pushDebugGroup("FinalFinalizationRenderer::render");

    gl::manager->setEnabled({});

    quad->bind();
    shader->bind();

    fboSampler->bind(0);
    sdr_color->bind(0);

    auto &settings = Game::get().debugSettings.rendering;

    shader->fragmentStage()->setUniform("u_vignette_params", glm::vec4(settings.vignette.factor, settings.vignette.inner, settings.vignette.outer, settings.vignette.sharpness));

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    gl::popDebugGroup();
}