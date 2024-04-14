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

   public:
    SkyRenderer();
    ~SkyRenderer();

    void render(Camera &camera);
};