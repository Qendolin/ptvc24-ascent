#include "MotionBlurRenderer.h"

#include "../Camera.h"
#include "../GL/Framebuffer.h"
#include "../GL/Geometry.h"
#include "../GL/Shader.h"
#include "../GL/StateManager.h"
#include "../GL/Texture.h"
#include "../Game.h"
#include "../Input.h"
#include "../Settings.h"

MotionBlurRenderer::MotionBlurRenderer() {
    shader = new gl::ShaderPipeline(
        {new gl::ShaderProgram("assets/shaders/quad_uv.vert"),
         new gl::ShaderProgram("assets/shaders/motion_blur.frag")});
    shader->setDebugLabel("motion_blur_renderer/shader");

    gl::Buffer *vbo = new gl::Buffer();
    vbo->setDebugLabel("motion_blur_renderer/vbo");
    glm::vec2 quad_verts[] = {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}};
    vbo->allocate(&quad_verts, sizeof(quad_verts), 0);

    quad = new gl::VertexArray();
    quad->setDebugLabel("motion_blur_renderer/vao");
    quad->layout(0, 0, 2, GL_FLOAT, false, 0);
    quad->bindBuffer(0, *vbo, 0, 2 * 4);
    quad->own(vbo);

    fboSampler = new gl::Sampler();
    fboSampler->setDebugLabel("motion_blur_renderer/fbo_sampler");
    fboSampler->wrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0);
    fboSampler->filterMode(GL_LINEAR, GL_LINEAR);
}

MotionBlurRenderer::~MotionBlurRenderer() {
    delete shader;
    delete quad;
    delete fboSampler;
    delete prevFrame;
    delete currFrame;
}

void MotionBlurRenderer::createTextures_() {
    delete prevFrame;
    prevFrame = new gl::Texture(GL_TEXTURE_2D);
    prevFrame->setDebugLabel("motion_blur_renderer/frame_a");
    prevFrame->allocate(1, GL_RGB8, viewport_.x, viewport_.y);
    delete currFrame;
    currFrame = new gl::Texture(GL_TEXTURE_2D);
    currFrame->setDebugLabel("motion_blur_renderer/frame_b");
    currFrame->allocate(1, GL_RGB8, viewport_.x, viewport_.y);
}

void MotionBlurRenderer::render(Camera &camera, gl::Framebuffer *color_fbo, gl::Texture *depth) {
    auto gameSettings = Game::get().settings.get();
    auto &settings = Game::get().debugSettings.rendering.motionBlur;
    if (gameSettings.motionBlur == 0.0) return;

    gl::pushDebugGroup("MotionBlurRenderer::render");

    gl::manager->setEnabled({});

    // glNamedFramebufferReadBuffer doesn't work??
    color_fbo->bind(GL_READ_FRAMEBUFFER);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glCopyTextureSubImage2D(currFrame->id(), 0, 0, 0, 0, 0, currFrame->width(), currFrame->height());

    quad->bind();
    shader->bind();

    fboSampler->bind(0);
    prevFrame->bind(0);
    fboSampler->bind(1);
    depth->bind(1);

    float fps = 1.0f / Game::get().input->timeDelta();
    float blur_scale = fps / settings.targetFps * gameSettings.motionBlur;
    shader->fragmentStage()->setUniform("u_prev_mvp_mat", prevMvpMat);
    shader->fragmentStage()->setUniform("u_motion_blur_scale", blur_scale);
    shader->fragmentStage()->setUniform("u_inverse_projection_mat", glm::inverse(camera.projectionMatrix()));
    shader->fragmentStage()->setUniform("u_inverse_view_mat", glm::inverse(camera.viewMatrix()));

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    prevMvpMat = camera.viewProjectionMatrix();
    std::swap(prevFrame, currFrame);

    gl::popDebugGroup();
}