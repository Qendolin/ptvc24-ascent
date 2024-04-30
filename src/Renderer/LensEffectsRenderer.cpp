#include "LensEffectsRenderer.h"

#include "../Camera.h"
#include "../GL/Geometry.h"
#include "../GL/Shader.h"
#include "../GL/StateManager.h"
#include "../GL/Texture.h"
#include "../Game.h"
#include "../Loader/Loader.h"

LensEffectsRenderer::LensEffectsRenderer() {
    flareShader = new gl::ShaderPipeline(
        {new gl::ShaderProgram("assets/shaders/lens_flare.comp")});
    flareShader->setDebugLabel("lens_effects_renderer/flare_shader");

    blurShader = new gl::ShaderPipeline(
        {new gl::ShaderProgram("assets/shaders/lens_blur.comp")});
    blurShader->setDebugLabel("lens_effects_renderer/blur_shader");

    glareShader = new gl::ShaderPipeline(
        {new gl::ShaderProgram("assets/shaders/lens_glare.comp")});
    glareShader->setDebugLabel("lens_effects_renderer/lens_glare");

    ghostColorLut = loader::texture("assets/textures/ghost_color_gradient.png", {.mipmap = false, .srgb = false});
    ghostColorLut->setDebugLabel("lens_effects_renderer/ghost_color_lut");

    lutSampler = new gl::Sampler();
    lutSampler->setDebugLabel("lens_effects_renderer/lut_sampler");
    lutSampler->wrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0);
    lutSampler->filterMode(GL_LINEAR, GL_LINEAR);

    effectSampler = new gl::Sampler();
    effectSampler->setDebugLabel("lens_effects_renderer/effect_sampler");
    effectSampler->wrapMode(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER, 0);
    effectSampler->borderColor(glm::vec4(0));
    effectSampler->filterMode(GL_LINEAR, GL_LINEAR);

    blurSampler = new gl::Sampler();
    blurSampler->setDebugLabel("lens_effects_renderer/blur_sampler");
    blurSampler->wrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0);
    blurSampler->filterMode(GL_LINEAR, GL_LINEAR);
}

LensEffectsRenderer::~LensEffectsRenderer() {
    delete flareShader;
    delete blurShader;
    delete glareShader;
    delete lutSampler;
    delete effectSampler;
    delete blurSampler;
    delete flares1;
    delete flares2;
    delete glare1;
    delete glare2;
    delete glareCombine;
    delete ghostColorLut;
}

void LensEffectsRenderer::createTextures_() {
    delete flares1;
    delete flares2;
    delete glare1;
    delete glare2;
    delete glareCombine;
    flares1 = new gl::Texture(GL_TEXTURE_2D);
    flares1->setDebugLabel("lens_effects_renderer/flares1");
    flares1->allocate(1, GL_R11F_G11F_B10F, viewport_.x / 2, viewport_.y / 2);
    flares2 = new gl::Texture(GL_TEXTURE_2D);
    flares2->setDebugLabel("lens_effects_renderer/flares2");
    flares2->allocate(1, GL_R11F_G11F_B10F, viewport_.x / 2, viewport_.y / 2);
    glare1 = new gl::Texture(GL_TEXTURE_2D);
    glare1->setDebugLabel("lens_effects_renderer/glare1");
    glare1->allocate(1, GL_R11F_G11F_B10F, viewport_.x / 4, viewport_.y / 4);
    glare2 = new gl::Texture(GL_TEXTURE_2D);
    glare2->setDebugLabel("lens_effects_renderer/glare1");
    glare2->allocate(1, GL_R11F_G11F_B10F, viewport_.x / 4, viewport_.y / 4);
    glareCombine = new gl::Texture(GL_TEXTURE_2D);
    glareCombine->setDebugLabel("lens_effects_renderer/glare_combine");
    glareCombine->allocate(1, GL_R11F_G11F_B10F, viewport_.x / 4, viewport_.y / 4);
}

// integer divide x / y but round up instead of truncate
#define DIV_CEIL(x, y) ((x + y - 1) / y)

void LensEffectsRenderer::render(gl::Texture* bloom_half, gl::Texture* bloom_quater) {
    gl::pushDebugGroup("LensEffectsRenderer::render");

    auto settings = Game::get().debugSettings.rendering.lens;
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

    // ghosts & halo
    if (settings.factor > 0.0f) {
        effectSampler->bind(0);
        bloom_half->bind(0);
        lutSampler->bind(1);
        ghostColorLut->bind(1);

        flareShader->bind();
        flareShader->get(GL_COMPUTE_SHADER)->setUniform("u_ghost_count", settings.ghosts);
        flareShader->get(GL_COMPUTE_SHADER)->setUniform("u_ghost_params", glm::vec4(settings.ghostDispersion, settings.ghostBias, settings.ghostFactor, 0.0));
        flareShader->get(GL_COMPUTE_SHADER)->setUniform("u_halo_params", glm::vec4(settings.haloSize, settings.haloBias, settings.haloFactor, 0.0));
        flareShader->get(GL_COMPUTE_SHADER)->setUniform("u_chromatic_distortion_fac", settings.chromaticDistortion);

        int w = viewport_.x / 2;
        int h = viewport_.y / 2;
        glBindImageTexture(0, flares1->id(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R11F_G11F_B10F);
        glDispatchCompute(DIV_CEIL(w, 8), DIV_CEIL(h, 8), 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        if (settings.blur) {
            // horizontal blur
            blurShader->bind();
            blurSampler->bind(0);
            flares1->bind(0);
            glBindImageTexture(0, flares2->id(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R11F_G11F_B10F);
            blurShader->get(GL_COMPUTE_SHADER)->setUniform("u_direction", glm::vec2(1.0, 0.0));
            glDispatchCompute(DIV_CEIL(w, 8), DIV_CEIL(h, 8), 1);
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

            // vertical blur
            flares2->bind(0);
            glBindImageTexture(0, flares1->id(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R11F_G11F_B10F);
            blurShader->get(GL_COMPUTE_SHADER)->setUniform("u_direction", glm::vec2(0.0, 1.0));
            glDispatchCompute(DIV_CEIL(w, 8), DIV_CEIL(h, 8), 1);
        }
    }

    // glare
    if (settings.factor * settings.glareFactor > 0.0f) {
        int w = viewport_.x / 4;
        int h = viewport_.y / 4;
        glareShader->bind();
        glareShader->get(GL_COMPUTE_SHADER)->setUniform("u_params", glm::vec3(settings.glareAttenuation, settings.glareBias, settings.glareFactor));

        for (int dir = 0; dir < GLARE_DIRECTIONS.size(); dir++) {
            gl::Texture* read = bloom_quater;
            gl::Texture* write = glare1;
            glareShader->get(GL_COMPUTE_SHADER)->setUniform("u_direction", GLARE_DIRECTIONS[dir]);
            for (int i = 0; i < GLARE_PASSES; i++) {
                bool combine = false;
                if (i == GLARE_PASSES - 1) {
                    write = glareCombine;
                    combine = dir > 0;  // combine at the last iteration, except for the first direction
                }

                glareShader->get(GL_COMPUTE_SHADER)->setUniform("u_iteration_params", glm::ivec2(i, combine));
                glBindImageTexture(0, read->id(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_R11F_G11F_B10F);
                glBindImageTexture(1, write->id(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R11F_G11F_B10F);
                glDispatchCompute(DIV_CEIL(w, 8), DIV_CEIL(h, 8), 1);
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

                if (write == glare1) {
                    write = glare2;
                    read = glare1;
                } else {
                    write = glare1;
                    read = glare2;
                }
            }
        }
    }

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);  // I have no clue

    gl::popDebugGroup();
}