#include <glm/glm.hpp>

#include "../GL/Declarations.h";

class TerrainRenderer {
   private:
    gl::ShaderPipeline *shader;
    // first, configure the cube's VAO (and terrainVBO + terrainIBO)
    unsigned int terrainVAO, terrainVBO, terrainIBO;
    int width, height, nrChannels;
    int rez = 1;

   public:
    void render(glm::mat4 viewProjectionMatrix);
    TerrainRenderer();
    ~TerrainRenderer();
};