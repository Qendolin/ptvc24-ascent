#include <glm/glm.hpp>

#include "../GL/Declarations.h"

class TessellationRenderer {
   private:
    gl::ShaderPipeline* shader;
    // first, configure the cube's VAO (and terrainVBO + terrainIBO)
    gl::VertexArray* terrainVAO;
    gl::Buffer* terrainVBO;
    const unsigned int NUM_PATCH_PTS = 4;
    int width, height, nrChannels;
    unsigned int rez = 20;

   public:
    void render(glm::mat4 viewProjectionMatrix, glm::mat4 viewMatrix);
    TessellationRenderer();
    ~TessellationRenderer();
};