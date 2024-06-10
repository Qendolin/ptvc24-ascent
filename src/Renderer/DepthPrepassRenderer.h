#pragma once

#include "../Camera.h"
#include "../GL/Declarations.h"

#pragma region ForwardDecl
class Camera;
namespace loader {
class GraphicsData;
class Terrain;
}  // namespace loader
#pragma endregion

class DepthPrepassRenderer {
   private:
    gl::ShaderPipeline* objectShader;
    gl::ShaderPipeline* terrainShader;
    gl::Sampler* terrainSampler;

   public:
    DepthPrepassRenderer();
    ~DepthPrepassRenderer();

    void render(Camera& camera, loader::GraphicsData& graphics, loader::Terrain& terrain);
};