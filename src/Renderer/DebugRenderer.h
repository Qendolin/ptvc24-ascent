#pragma once

#include <glm/glm.hpp>

#pragma region ForwardDecl
#include "../GL/Declarations.h"
class Camera;
class Game;
#pragma endregion

class DebugRenderer {
   private:
    gl::VertexArray* quad;
    gl::ShaderPipeline* shader;
    gl::Sampler* sampler;

   public:
    DebugRenderer();
    ~DebugRenderer();

    void render(Game& game);
};