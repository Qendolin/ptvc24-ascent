#pragma once

#include <array>
#include <glm/glm.hpp>

#pragma region ForwardDecl
#include "../GL/Declarations.h"
class Camera;
namespace loader {
class GraphicsData;
}  // namespace loader
#pragma endregion

class BloomRenderer {
   private:
    static const int LEVELS = 6;
    static const int LOCAL_GROUP_SIZE = 8;  // 8x8x1 = 64 local work group size is supported on all systems

    gl::ShaderPipeline *upShader;
    gl::ShaderPipeline *downShader;
    gl::Sampler *downSampler;
    gl::Sampler *upSampler;
    gl::Texture *upTexture = nullptr;
    std::array<gl::Texture *, LEVELS> upViews = {};
    gl::Texture *downTexture = nullptr;
    std::array<gl::Texture *, LEVELS> downViews = {};

    glm::ivec2 viewport_ = glm::ivec2(0, 0);

    void createTextures_();

   public:
    BloomRenderer();
    ~BloomRenderer();

    void render(gl::Texture *hrd_color);

    void setViewport(int width, int height) {
        if (width == viewport_.x && height == viewport_.y)
            return;
        viewport_ = {width, height};
        createTextures_();
    }

    gl::Texture *result() {
        return upViews[0];
    }
};