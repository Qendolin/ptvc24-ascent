#pragma once

#include <glm/glm.hpp>

#include "../Scene/Light.h"

#pragma region FrowardDecl
#include "../GL/Declarations.h"
class Camera;
class CSM;
namespace loader {
class Water;
class Environment;
}  // namespace loader
#pragma endregion

class WaterRenderer {
   private:
    gl::ShaderPipeline* shader;
    gl::Sampler* waterSampler;
    gl::Sampler* depthSampler;
    gl::Sampler* shadowSampler;

   public:
    WaterRenderer();
    ~WaterRenderer();

    void render(Camera& camera, loader::Water& water, loader::Environment& env, OrthoLight& sun, gl::Texture* depth);
};