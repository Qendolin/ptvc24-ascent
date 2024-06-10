#pragma once

#include <glm/glm.hpp>

#pragma region FrowardDecl
#include "../GL/Declarations.h"
class Camera;
class CSM;
struct OrthoLight;
namespace loader {
class Terrain;
class Environment;
}  // namespace loader
#pragma endregion

class TerrainRenderer {
   private:
    gl::ShaderPipeline* shader;
    gl::Sampler* terrainSampler;
    gl::Sampler* shadowSampler;

   public:
    TerrainRenderer();
    ~TerrainRenderer();

    void render(Camera& camera, loader::Terrain& terrain, CSM& csm, loader::Environment& env, OrthoLight& sun);
};