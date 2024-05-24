#include "GtaoRenderer.h"

#include "../Camera.h"
#include "../GL/Shader.h"
#include "../GL/StateManager.h"
#include "../GL/Texture.h"

// integer divide x / y but round up instead of truncate
#define DIV_CEIL(x, y) ((x + y - 1) / y)

// Constant for the width of the Hilbert curve
static const uint16_t HILBERT_WIDTH = 64;

// https://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/

// Function to compute the Hilbert index for given x and y coordinates
// From https://www.shadertoy.com/view/3tB3z3 - except we're using R2 here
static uint32_t hilbert_index(uint32_t posX, uint32_t posY) {
    uint32_t index = 0U;
    for (uint32_t curLevel = HILBERT_WIDTH / 2U; curLevel > 0U; curLevel /= 2U) {
        uint32_t regionX = (posX & curLevel) > 0U;
        uint32_t regionY = (posY & curLevel) > 0U;
        index += curLevel * curLevel * ((3U * regionX) ^ regionY);
        if (regionY == 0U) {
            if (regionX == 1U) {
                posX = uint32_t((HILBERT_WIDTH - 1U)) - posX;
                posY = uint32_t((HILBERT_WIDTH - 1U)) - posY;
            }

            uint32_t temp = posX;
            posX = posY;
            posY = temp;
        }
    }
    return index;
}

// Function to generate a lookup table for Hilbert indices
static std::array<uint16_t, HILBERT_WIDTH * HILBERT_WIDTH> generate_hilbert_index_lut() {
    std::array<uint16_t, HILBERT_WIDTH * HILBERT_WIDTH> result = {};

    for (uint16_t y = 0; y < HILBERT_WIDTH; y++) {
        for (uint16_t x = 0; x < HILBERT_WIDTH; x++) {
            uint32_t index = hilbert_index(x, y);
            assert(index < 65536);
            result[x + y * HILBERT_WIDTH] = static_cast<uint16_t>(index);
        }
    }

    return result;
}

GtaoRenderer::GtaoRenderer() {
    depthShader = new gl::ShaderPipeline({new gl::ShaderProgram("assets/shaders/gtao/preprocess_depth.comp")});
    depthShader->setDebugLabel("gtao_renderer/depth_shader");

    gtaoShader = new gl::ShaderPipeline({new gl::ShaderProgram("assets/shaders/gtao/gtao.comp")});
    gtaoShader->setDebugLabel("gtao_renderer/gtao_shader");

    denoiseShader = new gl::ShaderPipeline({new gl::ShaderProgram("assets/shaders/gtao/spatial_denoise.comp")});
    denoiseShader->setDebugLabel("gtao_renderer/denoise_shader");

    sampler = new gl::Sampler();
    sampler->setDebugLabel("gtao_renderer/sampler");
    sampler->filterMode(GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST);
    sampler->wrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

    hilbertLut = new gl::Texture(GL_TEXTURE_2D);
    hilbertLut->allocate(1, GL_R16UI, 64, 64);
    auto hilbert_lut = generate_hilbert_index_lut();
    hilbertLut->load(0, HILBERT_WIDTH, HILBERT_WIDTH, GL_RED_INTEGER, GL_UNSIGNED_SHORT, &hilbert_lut[0]);
}

GtaoRenderer::~GtaoRenderer() {
    delete depthShader;
    delete gtaoShader;
    delete denoiseShader;
    delete sampler;
    delete depthMips;
    delete noisyOcclusion;
    delete noisyEdges;
    delete filteredOcclusion;
    delete hilbertLut;
}

void GtaoRenderer::createTextures_() {
    delete depthMips;
    delete noisyOcclusion;
    delete noisyEdges;
    delete filteredOcclusion;

    depthMips = new gl::Texture(GL_TEXTURE_2D);
    depthMips->setDebugLabel("gtao_renderer/depth_mips");
    depthMips->allocate(5, GL_R16F, viewport_.x, viewport_.y);

    noisyOcclusion = new gl::Texture(GL_TEXTURE_2D);
    noisyOcclusion->setDebugLabel("gtao_renderer/noisy_occlusion");
    // noisyOcclusion->allocate(5, GL_R16F, viewport_.x, viewport_.y);
    noisyOcclusion->allocate(5, GL_RGBA16F, viewport_.x, viewport_.y);

    noisyEdges = new gl::Texture(GL_TEXTURE_2D);
    noisyEdges->setDebugLabel("gtao_renderer/noisy_edges");
    noisyEdges->allocate(5, GL_R32UI, viewport_.x, viewport_.y);

    filteredOcclusion = new gl::Texture(GL_TEXTURE_2D);
    filteredOcclusion->setDebugLabel("gtao_renderer/filtered_occlusion");
    filteredOcclusion->allocate(5, GL_R16F, viewport_.x, viewport_.y);
}

void GtaoRenderer::render(Camera &camera, gl::Texture &depth_texture, gl::Texture &view_normals_texture) {
    gl::pushDebugGroup("GtaoRenderer::render");
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

    int w = viewport_.x;
    int h = viewport_.y;

    sampler->bind(0);
    sampler->bind(1);

    depthShader->bind();
    depth_texture.bind(0);
    glBindImageTexture(0, depthMips->id(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R16F);
    glBindImageTexture(1, depthMips->id(), 1, GL_FALSE, 0, GL_WRITE_ONLY, GL_R16F);
    glBindImageTexture(2, depthMips->id(), 2, GL_FALSE, 0, GL_WRITE_ONLY, GL_R16F);
    glBindImageTexture(3, depthMips->id(), 3, GL_FALSE, 0, GL_WRITE_ONLY, GL_R16F);
    glBindImageTexture(4, depthMips->id(), 4, GL_FALSE, 0, GL_WRITE_ONLY, GL_R16F);
    glDispatchCompute(DIV_CEIL(w, 8), DIV_CEIL(h, 8), 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    gtaoShader->bind();
    gtaoShader->get(GL_COMPUTE_SHADER)->setUniform("u_frame", frame++);
    gtaoShader->get(GL_COMPUTE_SHADER)->setUniform("u_inverse_view_mat", glm::inverse(camera.viewMatrix()));
    gtaoShader->get(GL_COMPUTE_SHADER)->setUniform("u_inverse_projection_mat", glm::inverse(camera.projectionMatrix()));
    gtaoShader->get(GL_COMPUTE_SHADER)->setUniform("u_projection_mat", camera.projectionMatrix());

    depthMips->bind(0);
    view_normals_texture.bind(1);
    // glBindImageTexture(0, noisyOcclusion->id(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R16F);
    glBindImageTexture(0, noisyOcclusion->id(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    glBindImageTexture(1, noisyEdges->id(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32UI);
    glBindImageTexture(2, hilbertLut->id(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_R16UI);
    glDispatchCompute(DIV_CEIL(w, 8), DIV_CEIL(h, 8), 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    denoiseShader->bind();
    noisyOcclusion->bind(0);
    noisyEdges->bind(1);
    glBindImageTexture(0, filteredOcclusion->id(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R16F);
    glDispatchCompute(DIV_CEIL(w, 8), DIV_CEIL(h, 8), 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    gl::popDebugGroup();
}
