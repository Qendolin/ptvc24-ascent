#pragma once

#include <memory>

#pragma region ForwardDecl
#include "../GL/Declarations.h"
class Camera;
class ShadowCaster;
class IblEnvironment;
namespace loader {
class GraphicsData;
}
#pragma endregion

class MaterialBatchRenderer {
   private:
    gl::ShaderPipeline *shader;
    gl::Sampler *albedoSampler;
    gl::Sampler *normalSampler;
    gl::Sampler *ormSampler;
    gl::Sampler *shadowSampler;

   public:
    MaterialBatchRenderer();
    ~MaterialBatchRenderer();

    void render(Camera &camera, loader::GraphicsData &graphics, ShadowCaster &shadow, IblEnvironment &env);
};