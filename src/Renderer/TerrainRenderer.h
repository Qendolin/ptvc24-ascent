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
    gl::Texture* height;
    gl::Texture* albedo;
    gl::Texture* normal;
    gl::Texture* occlusion;
    gl::Sampler* sampler;
    int resolution = 20;

   public:
    void render(Camera& camera, IblEnvironment& env);
    TerrainRenderer();
    ~TerrainRenderer();
};