#pragma once

#include <glm/glm.hpp>

#pragma region ForwardDecl
#include "../GL/Declarations.h"
class Camera;
#pragma endregion

class MotionBlurRenderer {
    gl::ShaderPipeline *shader;
    // A quad with dimensions(-1, -1) to(1, 1)
    gl::VertexArray *quad;
    gl::Sampler *fboSampler;

    glm::mat4 prevMvpMat = glm::mat4(1.0f);
    gl::Texture *prevFrame = nullptr;
    gl::Texture *currFrame = nullptr;

    glm::ivec2 viewport_ = glm::ivec2(0, 0);

    void createTextures_();

   public:
    MotionBlurRenderer();
    ~MotionBlurRenderer();

    void setViewport(int width, int height) {
        if (width == viewport_.x && height == viewport_.y)
            return;
        viewport_ = {width, height};
        createTextures_();
    }

    void render(Camera &camera, gl::Framebuffer *color_fbo, gl::Texture *depth);
};