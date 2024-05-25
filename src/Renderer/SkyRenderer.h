#pragma once

#include <memory>

#pragma region ForwardDecl
#include "../GL/Declarations.h"
class Camera;
namespace loader {
class Environment;
}
#pragma endregion

class SkyRenderer {
   private:
    gl::ShaderPipeline *shader;
    // A quad with dimensions(-1, -1) to(1, 1)
    gl::VertexArray *cube;

   public:
    SkyRenderer();
    ~SkyRenderer();

    void render(Camera &camera, loader::Environment &env);
};