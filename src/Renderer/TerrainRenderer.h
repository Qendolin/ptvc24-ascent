#include <glm/glm.hpp>

#pragma region FrowardDecl
#include "../GL/Declarations.h"
class Camera;
#pragma endregion

class TerrainRenderer {
   private:
    gl::ShaderPipeline* shader;
    gl::VertexArray* vao;
    gl::Texture* heightMap;
    gl::Sampler* sampler;
    int resolution = 20;

   public:
    void render(Camera& camera);
    TerrainRenderer();
    ~TerrainRenderer();
};