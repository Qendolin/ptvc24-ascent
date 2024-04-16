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
    gl::Texture *iblDiffuse;
    gl::Texture *iblSpecular;
    gl::Texture *iblBrdfLut;
    gl::Sampler *albedoSampler;
    gl::Sampler *normalSampler;
    gl::Sampler *ormSampler;
    gl::Sampler *lutSampler;
    gl::Sampler *cubemapSampler;

   public:
    MaterialBatchRenderer();
    ~MaterialBatchRenderer();

    void render(Camera &camera, loader::GraphicsData &graphics);
};