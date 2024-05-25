#include <glm/glm.hpp>

#pragma region FrowardDecl
#include "../GL/Declarations.h"
class Camera;
namespace loader {
class Terrain;
class Environment;
}
#pragma endregion

class TerrainRenderer {
   private:
    gl::ShaderPipeline* shader;
    gl::Sampler* sampler;

   public:
    TerrainRenderer();
    ~TerrainRenderer();

    void render(Camera& camera, loader::Terrain& terrain, loader::Environment& env);
};