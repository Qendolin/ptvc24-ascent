#pragma once

#pragma region ForwardDecl
#include "../GL/Declarations.h"
class Camera;
namespace loader {
class GraphicsData;
}  // namespace loader
#pragma endregion

class MaterialBatchRenderer {
   private:
    gl::ShaderPipeline *shader;

   public:
    MaterialBatchRenderer();
    ~MaterialBatchRenderer();

    void render(Camera &camera, loader::GraphicsData &graphics);
};