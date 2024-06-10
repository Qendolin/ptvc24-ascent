#include <glm/glm.hpp>

#pragma region FrowardDecl
#include "../GL/Declarations.h"
class Camera;
class CSM;
namespace loader {
class Water;
class Environment;
}  // namespace loader
#pragma endregion

class WaterTRenderer {
   private:
    gl::ShaderPipeline* shader;
    gl::Sampler* waterSampler;
    gl::Sampler* shadowSampler;

   public:
    WaterTRenderer();
    ~WaterTRenderer();

    void render(Camera& camera, loader::Water& water, CSM& csm, loader::Environment& env);
};