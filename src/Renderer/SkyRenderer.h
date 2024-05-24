#pragma once

#include <memory>

#pragma region ForwardDecl
#include "../GL/Declarations.h"
class Camera;
namespace loader {
class IblEnv;
struct FloatImage;
}  // namespace loader
#pragma endregion

class SkyRenderer {
   private:
    gl::ShaderPipeline *shader;
    // A quad with dimensions(-1, -1) to(1, 1)
    gl::VertexArray *cube;
    gl::Sampler *sampler;
    gl::Texture *cubemap;

   public:
    SkyRenderer(std::shared_ptr<loader::IblEnv> environment);
    ~SkyRenderer();

    void render(Camera &camera);
};