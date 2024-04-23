#include "BloomRenderer.h"

#include "../Camera.h"
#include "../GL/Geometry.h"
#include "../GL/Shader.h"
#include "../GL/StateManager.h"
#include "../GL/Texture.h"
#include "../Game.h"

BloomRenderer::BloomRenderer() {
    upShader = new gl::ShaderPipeline(
        {new gl::ShaderProgram("assets/shaders/bloom_up.comp")});
    upShader->setDebugLabel("bloom_renderer/up_shader");

    downShader = new gl::ShaderPipeline(
        {new gl::ShaderProgram("assets/shaders/bloom_down.comp")});
    downShader->setDebugLabel("bloom_renderer/down_shader");

    // https://www.froyok.fr/blog/2021-12-ue4-custom-bloom/#:~:text=This%20hybrid%20mode%20still%20gives%20a%20smooth%20fade%20at%20the%20edges%20when%20the%20bright%20area%20disappear
    downSampler = new gl::Sampler();
    downSampler->setDebugLabel("bloom_renderer/down_sampler");
    downSampler->wrapMode(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER, 0);
    downSampler->borderColor(glm::vec4(0));
    downSampler->filterMode(GL_LINEAR, GL_LINEAR);

    upSampler = new gl::Sampler();
    upSampler->setDebugLabel("bloom_renderer/down_sampler");
    upSampler->wrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, 0);
    upSampler->filterMode(GL_LINEAR, GL_LINEAR);
}

BloomRenderer::~BloomRenderer() {
    delete upShader;
    delete downShader;
    delete downSampler;
    delete upSampler;
    for (int i = 0; i < LEVELS; i++) {
        delete upViews[i];
        delete downViews[i];
    }
    delete upTexture;
    delete downTexture;
}

void BloomRenderer::createTextures_() {
    delete upTexture;
    delete downTexture;
    for (int i = 0; i < LEVELS; i++) {
        delete upViews[i];
        delete downViews[i];
    }

    upTexture = new gl::Texture(GL_TEXTURE_2D);
    upTexture->setDebugLabel("bloom_renderer/up_texture");
    upTexture->allocate(LEVELS, GL_R11F_G11F_B10F, viewport_.x, viewport_.y);
    for (int i = 0; i < LEVELS; i++) {
        upViews[i] = upTexture->createView(GL_TEXTURE_2D, GL_R11F_G11F_B10F, i, i, 0, 0);
        upViews[i]->setDebugLabel("bloom_renderer/up_view[" + std::to_string(i) + "]");
    }

    downTexture = new gl::Texture(GL_TEXTURE_2D);
    downTexture->allocate(LEVELS, GL_R11F_G11F_B10F, viewport_.x / 2, viewport_.y / 2);
    downTexture->setDebugLabel("bloom_renderer/down_texture");
    for (int i = 0; i < LEVELS; i++) {
        downViews[i] = downTexture->createView(GL_TEXTURE_2D, GL_R11F_G11F_B10F, i, i, 0, 0);
        downViews[i]->setDebugLabel("bloom_renderer/down_view[" + std::to_string(i) + "]");
    }
}

//		Up		Down
//	0=	1		1/2
//	1=	1/2		1/4
//	2=	1/4		1/8
//	3=	1/8		1/16
//	4=	1/16	1/32
//	5=	1/32	1/64
//
//	Down:
//
//	down(src) -> Down[0]
//	down(Down[1]) -> Down[2]
//	down(Down[2]) -> Down[3]
//	down(Down[3]) -> Down[4]
//	down(Down[4]) -> Down[5]
//
//	Up:
//
//	Down[4] + up(Down[5]) -> Up[5]
//	Down[3] + up(Up[5]) -> Up[4]
//	Down[2] + up(Up[4]) -> Up[3]
//	Down[1] + up(Up[3]) -> Up[2]
//	Down[0] + up(Up[2]) -> Up[1]
//	up(Up[1]) -> Up[0]

// https://www.froyok.fr/blog/2021-12-ue4-custom-bloom/
// https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare/
// https://learnopengl.com/Guest-Articles/2022/Phys.-Based-Bloom Note: Incorrect

// integer divide x / y but round up instead of truncate
#define DIV_CEIL(x, y) ((x + y - 1) / y)

void BloomRenderer::render(gl::Texture* hrd_color) {
    gl::pushDebugGroup("BloomRenderer::render");

    hrd_color->bind(0);

    auto settings = Game::get().debugSettings.rendering.bloom;

    float threshold = settings.threshold;
    float knee = settings.thresholdKnee;
    knee = std::max(threshold * knee, 1e-5f);
    downShader->bind();
    downShader->get(GL_COMPUTE_SHADER)->setUniform("u_threshold", glm::vec4(threshold, threshold - knee, knee * 2.0, 0.25 / knee));
    downShader->get(GL_COMPUTE_SHADER)->setUniform("u_first_pass", 1);
    downSampler->bind(0);

    int w = viewport_.x / 2;
    int h = viewport_.y / 2;
    glBindImageTexture(0, downTexture->id(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R11F_G11F_B10F);
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);  // not sure if this is the correct barrier
    glDispatchCompute(DIV_CEIL(w, 8), DIV_CEIL(h, 8), 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);  // Maybe also GL_TEXTURE_FETCH_BARRIER_BIT the documentation is really unclear ?

    downShader->get(GL_COMPUTE_SHADER)->setUniform("u_first_pass", 0);
    for (int i = 0; i < LEVELS - 1; i++) {
        downViews[i]->bind(0);  // bind the previous level as input for the next
        w /= 2;
        h /= 2;
        glBindImageTexture(0, downTexture->id(), i + 1, GL_FALSE, 0, GL_WRITE_ONLY, GL_R11F_G11F_B10F);
        glDispatchCompute(DIV_CEIL(w, LOCAL_GROUP_SIZE), DIV_CEIL(h, LOCAL_GROUP_SIZE), 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    upShader->bind();
    upSampler->bind(0);
    gl::Texture* lower_view = downViews.back();  // start with lowest level
    for (int i = LEVELS - 1; i >= 0; i--) {
        if (i == 0) {
            // the full res top level doesn't get any contribution from the downsampling
            upShader->get(GL_COMPUTE_SHADER)->setUniform("u_factor", glm::vec2{1.0f, 0.0f});
        } else {
            if (i == LEVELS - 1) {
                upShader->get(GL_COMPUTE_SHADER)->setUniform("u_factor", glm::vec2{settings.levels[i], settings.levels[i - 1]});
            } else {
                upShader->get(GL_COMPUTE_SHADER)->setUniform("u_factor", glm::vec2{1.0f, settings.levels[i - 1]});
            }
            glBindImageTexture(0, downTexture->id(), i - 1, GL_FALSE, 0, GL_READ_ONLY, GL_R11F_G11F_B10F);
        }
        lower_view->bind(0);
        glBindImageTexture(1, upTexture->id(), i, GL_FALSE, 0, GL_WRITE_ONLY, GL_R11F_G11F_B10F);
        w = viewport_.x / (1 << i);
        h = viewport_.y / (1 << i);
        glDispatchCompute(DIV_CEIL(w, LOCAL_GROUP_SIZE), DIV_CEIL(h, LOCAL_GROUP_SIZE), 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        lower_view = upViews[i];
    }
    glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);  // I have no clue

    gl::popDebugGroup();
}