#pragma once

#pragma region ForwardDecl
#include "../GL/Declarations.h"
class Camera;
#pragma endregion

class FinalFinalizationRenderer {
   private:
    gl::ShaderPipeline *shader;
    // A quad with dimensions(-1, -1) to(1, 1)
    gl::VertexArray *quad;
    gl::Sampler *fboSampler;

   public:
    FinalFinalizationRenderer();
    ~FinalFinalizationRenderer();

    void render(gl::Texture *sdr_color);
};