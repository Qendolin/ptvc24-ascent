#include "TessellationRenderer.h"

#include <glm/glm.hpp>
#include <iostream>

#include "../GL/Geometry.h"
#include "../GL/Shader.h"
#include "../GL/StateManager.h"
#include "../GL/Texture.h"
#include "stb_image.h"

TessellationRenderer::TessellationRenderer() {
    shader = new gl::ShaderPipeline(
        {new gl::ShaderProgram("assets/shaders/terrain.vert"),
         new gl::ShaderProgram("assets/shaders/terrain.frag"),
         new gl::ShaderProgram("assets/shaders/terrain.tesc"),
         new gl::ShaderProgram("assets/shaders/terrain.tese")});
    shader->setDebugLabel("Tessellation_renderer/shader");

    // load and create a texture
    // -------------------------
    unsigned int texture;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    gl::manager->bindTexture(GL_TEXTURE_2D, texture);  // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);  // set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    unsigned char *data = stbi_load("assets/textures/iceland_heightmap.png", &width, &height, &nrChannels, 0);
    if (data) {
        std::cout << "Loaded heightmap of size " << height << " x " << width << std::endl;
    } else {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);

    std::vector<float> vertices;
    std::cout << "values of width and height are: " << height << ", " << width << std::endl;
    for (unsigned int i = 0; i <= rez - 1; i++) {
        for (unsigned int j = 0; j <= rez - 1; j++) {
            vertices.push_back(-width / 2.0f + width * i / (float)rez);    // v.x
            vertices.push_back(0.0f);                                      // v.y
            vertices.push_back(-height / 2.0f + height * j / (float)rez);  // v.z
            vertices.push_back(i / (float)rez);                            // u
            vertices.push_back(j / (float)rez);                            // v

            vertices.push_back(-width / 2.0f + width * (i + 1) / (float)rez);  // v.x
            vertices.push_back(0.0f);                                          // v.y
            vertices.push_back(-height / 2.0f + height * j / (float)rez);      // v.z
            vertices.push_back((i + 1) / (float)rez);                          // u
            vertices.push_back(j / (float)rez);                                // v

            vertices.push_back(-width / 2.0f + width * i / (float)rez);          // v.x
            vertices.push_back(0.0f);                                            // v.y
            vertices.push_back(-height / 2.0f + height * (j + 1) / (float)rez);  // v.z
            vertices.push_back(i / (float)rez);                                  // u
            vertices.push_back((j + 1) / (float)rez);                            // v

            vertices.push_back(-width / 2.0f + width * (i + 1) / (float)rez);    // v.x
            vertices.push_back(0.0f);                                            // v.y
            vertices.push_back(-height / 2.0f + height * (j + 1) / (float)rez);  // v.z
            vertices.push_back((i + 1) / (float)rez);                            // u
            vertices.push_back((j + 1) / (float)rez);                            // v
        }
    }

    std::cout << "Loaded " << rez * rez << " patches of 4 control points each" << std::endl;
    std::cout << "Processing " << rez * rez * 4 << " vertices in vertex shader" << std::endl;

    // first, configure the cube's VAO (and terrainVBO)
    terrainVAO = new gl::VertexArray();
    terrainVAO->bind();

    terrainVBO = new gl::Buffer();
    terrainVBO->bind(GL_ARRAY_BUFFER);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    // texCoord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(sizeof(float) * 3));
    glEnableVertexAttribArray(1);

    glPatchParameteri(GL_PATCH_VERTICES, NUM_PATCH_PTS);
}

void TessellationRenderer::render(glm::mat4 viewProjectionMatrix, glm::mat4 viewMatrix) {
    gl::manager->polygonMode(GL_FRONT_AND_BACK, GL_LINE);
    gl::pushDebugGroup("TessellationRenderer::render");
    terrainVAO->bind();
    gl::manager->setEnabled({gl::Capability::DepthTest});
    gl::manager->depthFunc(gl::DepthFunc::GreaterOrEqual);
    gl::manager->depthMask(true);
    shader->bind();
    glDrawArrays(GL_PATCHES, 0, NUM_PATCH_PTS * rez * rez);
    shader->get(GL_TESS_EVALUATION_SHADER)->setUniform("u_projection_mat", viewProjectionMatrix);
    shader->get(GL_TESS_CONTROL_SHADER)->setUniform("u_view_mat", viewMatrix);
    gl::popDebugGroup();
    gl::manager->polygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

TessellationRenderer::~TessellationRenderer() {
    delete shader;
}