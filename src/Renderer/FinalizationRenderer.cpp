#include "FinalizationRenderer.h"

#include "../Camera.h"
#include "../GL/Geometry.h"
#include "../GL/Shader.h"
#include "../GL/StateManager.h"
#include "../GL/Texture.h"
#include "../Game.h"
#include "IblEnvironment.h"

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

    fboLinearSampler = new gl::Sampler();
    fboLinearSampler->setDebugLabel("finalization_renderer/fbo_linear_sampler");
    fboLinearSampler->wrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0);
    fboLinearSampler->filterMode(GL_LINEAR, GL_LINEAR);
}

FinalizationRenderer::~FinalizationRenderer() {
    delete shader;
    delete quad;
    delete fboSampler;
    delete fboLinearSampler;
}

void FinalizationRenderer::render(Camera &camera, gl::Texture *hrd_color, gl::Texture *depth, gl::Texture *bloom, gl::Texture *flares, gl::Texture *glare, gl::Texture *ao) {
    gl::pushDebugGroup("FinalizationRenderer::render");

    gl::manager->setEnabled({});

    quad->bind();
    shader->bind();

    fboSampler->bind(0);
    fboSampler->bind(1);
    fboLinearSampler->bind(2);
    fboLinearSampler->bind(3);
    fboLinearSampler->bind(4);
    fboLinearSampler->bind(5);

    hrd_color->bind(0);
    depth->bind(1);
    bloom->bind(2);
    flares->bind(3);
    glare->bind(4);
    ao->bind(5);

    auto &settings = Game::get().debugSettings.rendering;

    shader->fragmentStage()->setUniform("u_bloom_fac", settings.bloom.factor);
    shader->fragmentStage()->setUniform("u_flares_fac", settings.lens.factor);
    shader->fragmentStage()->setUniform("u_vignette_params", glm::vec4(settings.vignette.factor, settings.vignette.inner, settings.vignette.outer, settings.vignette.sharpness));
    shader->fragmentStage()->setUniform("u_inverse_projection_mat", glm::inverse(camera.projectionMatrix()));
    shader->fragmentStage()->setUniform("u_inverse_view_mat", glm::inverse(camera.viewMatrix()));
    shader->fragmentStage()->setUniform("u_camera_pos", camera.position);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    gl::popDebugGroup();
}