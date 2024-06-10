#include <glm/glm.hpp>

#pragma region FrowardDecl
#include "../GL/Declarations.h"
class Camera;
#pragma endregion

class WaterRenderer {
   private:
    gl::ShaderPipeline* shader;
    gl::VertexArray* vao;
    int rez = 1;
    int width;
    int height;
    gl::Texture* image;
    gl::Sampler* sampler;

   public:
    void render(Camera& camera);
    WaterRenderer();
    ~WaterRenderer();
};