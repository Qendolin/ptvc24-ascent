#include "TerrainRenderer.h"

#include <glm/glm.hpp>
#include <iostream>

#include "../GL/Shader.h"
#include "../GL/StateManager.h"
#include "stb_image.h"

TerrainRenderer::TerrainRenderer() {
    shader = new gl::ShaderPipeline(
        {new gl::ShaderProgram("assets/shaders/terrain.vert"),
         new gl::ShaderProgram("assets/shaders/terrain.frag")});
    shader->setDebugLabel("Terrain_Renderer/shader");

    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load("assets/textures/iceland_heightmap.png", &width, &height, &nrChannels, 0);
    if (data) {
        std::cout << "Loaded heightmap of size " << height << " x " << width << std::endl;
    } else {
        std::cout << "Failed to load texture" << std::endl;
    }

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    std::vector<float> vertices;
    float yScale = 64.0f / 256.0f, yShift = 16.0f;
    unsigned bytePerPixel = nrChannels;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            unsigned char* pixelOffset = data + (j + width * i) * bytePerPixel;
            unsigned char y = pixelOffset[0];

            // vertex
            vertices.push_back(-height / 2.0f + height * i / (float)height);  // vx
            vertices.push_back((int)y * yScale - yShift);                     // vy
            vertices.push_back(-width / 2.0f + width * j / (float)width);     // vz
        }
    }
    std::cout << "Loaded " << vertices.size() / 3 << " vertices" << std::endl;
    stbi_image_free(data);

    std::vector<unsigned int> indices;
    for (unsigned int i = 0; i < height - 1; i += rez) {
        for (unsigned int j = 0; j < width; j += rez) {
            for (int k = 0; k < 2; k++) {
                indices.push_back(j + width * (i + k * rez));
            }
        }
    }
    std::cout << "Loaded " << indices.size() << " indices" << std::endl;

    const int numStrips = (height - 1) / rez;
    const int numTrisPerStrip = (width / rez) * 2 - 2;
    std::cout << "Created lattice of " << numStrips << " strips with " << numTrisPerStrip << " triangles each" << std::endl;
    std::cout << "Created " << numStrips * numTrisPerStrip << " triangles total" << std::endl;

    glGenVertexArrays(1, &terrainVAO);
    gl::manager->bindVertexArray(terrainVAO);
    glGenBuffers(1, &terrainVBO);
    gl::manager->bindArrayBuffer(terrainVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &terrainIBO);
    gl::manager->bindElementArrayBuffer(terrainIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned), &indices[0], GL_STATIC_DRAW);
}

void TerrainRenderer::render(glm::mat4 viewProjectionMatrix) {
    const int numStrips = (height - 1) / rez;
    const int numTrisPerStrip = (width / rez) * 2 - 2;
    gl::manager->bindVertexArray(terrainVAO);
    gl::manager->setEnabled({gl::Capability::DepthTest});
    gl::manager->depthFunc(gl::DepthFunc::GreaterOrEqual);
    gl::manager->depthMask(true);
    shader->bind();
    for (unsigned int strip = 0; strip < numStrips; strip++) {
        glDrawElements(GL_TRIANGLE_STRIP,                                      // primitive type
                       numTrisPerStrip + 2,                                    // number of indices to render
                       GL_UNSIGNED_INT,                                        // index data type
                       (void*)(sizeof(int) * (numTrisPerStrip + 2) * strip));  // offset to starting index
    }
    shader->vertexStage()->setUniform("u_projection_mat", viewProjectionMatrix);
}

TerrainRenderer::~TerrainRenderer() {
    delete shader;
}
