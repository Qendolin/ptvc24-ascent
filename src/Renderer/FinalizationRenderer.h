#pragma once

#pragma region ForwardDecl
#include "../GL/Declarations.h"
class Camera;
#pragma endregion

class FinalizationRenderer {
   private:
    gl::ShaderPipeline *shader;
    // A quad with dimensions(-1, -1) to(1, 1)
    gl::VertexArray *quad;
    gl::Sampler *fboSampler;
    gl::Sampler *fboLinearSampler;

   public:
    FinalizationRenderer();
    ~FinalizationRenderer();

    void render(Camera &camera, gl::Texture *hrd_color, gl::Texture *depth, gl::Texture *bloom, gl::Texture *flares, gl::Texture *glare, gl::Texture *ao);
};