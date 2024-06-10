#pragma once

#include <memory>

#pragma region ForwardDecl
#include "../GL/Declarations.h"
class Camera;
class CSM;
struct OrthoLight;
namespace loader {
class GraphicsData;
class Environment;
}  // namespace loader
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

    void render(Camera &camera, loader::GraphicsData &graphics, CSM &csm, loader::Environment &env, OrthoLight &sun);
};