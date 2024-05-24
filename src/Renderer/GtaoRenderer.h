#pragma once

#include <glm/glm.hpp>

#pragma region ForwardDecl
#include "../GL/Declarations.h"
class Camera;
#pragma endregion

class GtaoRenderer {
   private:
    gl::ShaderPipeline *depthShader;
    gl::ShaderPipeline *gtaoShader;
    gl::ShaderPipeline *denoiseShader;
    gl::Sampler *sampler;
    gl::Texture *hilbertLut;
    gl::Texture *depthMips = nullptr;
    gl::Texture *noisyOcclusion = nullptr;
    gl::Texture *noisyEdges = nullptr;
    gl::Texture *filteredOcclusion = nullptr;
    uint32_t frame = 0;

    glm::ivec2 viewport_ = glm::ivec2(0, 0);

    void createTextures_();

   public:
    GtaoRenderer();
    ~GtaoRenderer();

    void setViewport(int width, int height) {
        if (width == viewport_.x && height == viewport_.y)
            return;
        viewport_ = {width, height};
        createTextures_();
    }

    gl::Texture *result() {
        return filteredOcclusion;
    }

    void render(Camera &camera, gl::Texture &depth_texture, gl::Texture &view_normals_texture);
    void debug(gl::Texture &out_texture, Camera &camera, gl::Texture &depth_texture, gl::Texture &view_normals_texture);
};