#include <glm/glm.hpp>

#pragma region FrowardDecl
#include "../GL/Declarations.h"
class Camera;
class IblEnvironment;
#pragma endregion

class TerrainRenderer {
   private:
    gl::ShaderPipeline* shader;
    gl::VertexArray* vao;
    gl::Texture* heightMap;
    gl::Sampler* heightSampler;
    int resolution = 20;

   public:
    void render(Camera& camera, IblEnvironment& env);
    TerrainRenderer();
    ~TerrainRenderer();
};