#pragma once

#include <array>
#include <glm/glm.hpp>

#pragma region ForwardDecl
#include "../GL/Declarations.h"
#pragma endregion

// References:
// https://john-chapman-graphics.blogspot.com/2013/02/pseudo-lens-flare.html
// https://www.froyok.fr/blog/2021-09-ue4-custom-lens-flare/
// https://www.rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/
// Light Streaks:
// https://www.chrisoat.com/papers/Oat-ScenePostprocessing.pdf
// http://www.daionet.gr.jp/~masa/archives/GDC2003_DSTEAL.ppt

class LensEffectsRenderer {
   private:
    static const int LOCAL_GROUP_SIZE = 8;  // 8x8x1 = 64 local work group size is supported on all systems
    static const constexpr std::array<glm::ivec2, 4> GLARE_DIRECTIONS = {glm::ivec2{-1, -1}, glm::ivec2{1, -1}, glm::ivec2{-1, 1}, glm::ivec2{1, 1}};
    static const int GLARE_PASSES = 4;

    gl::ShaderPipeline *flareShader;
    gl::ShaderPipeline *blurShader;
    gl::ShaderPipeline *glareShader;
    gl::Sampler *lutSampler;
    gl::Sampler *effectSampler;
    gl::Sampler *blurSampler;
    gl::Texture *flares1 = nullptr;
    gl::Texture *flares2 = nullptr;
    gl::Texture *glare1 = nullptr;
    gl::Texture *glare2 = nullptr;
    gl::Texture *glareCombine = nullptr;
    gl::Texture *ghostColorLut;

    glm::ivec2 viewport_ = glm::ivec2(0, 0);

    void createTextures_();

   public:
    LensEffectsRenderer();
    ~LensEffectsRenderer();

    void render(gl::Texture *bloom_half, gl::Texture *bloom_quater);

    void setViewport(int width, int height) {
        if (width == viewport_.x && height == viewport_.y)
            return;
        viewport_ = {width, height};
        createTextures_();
    }

    gl::Texture *flares() {
        return flares1;
    }

    gl::Texture *glare() {
        return glareCombine;
    }
};