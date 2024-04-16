#pragma once

#pragma region ForwardDecl
#include "../GL/Declarations.h"
class Camera;
#pragma endregion

class SkyRenderer {
   private:
    gl::ShaderPipeline *shader;
    // A quad with dimensions(-1, -1) to(1, 1)
    gl::VertexArray *quad;
    gl::Sampler *sampler;
    gl::Texture *cubemap;

   public:
    SkyRenderer();
    ~SkyRenderer();

    void render(Camera &camera);
};