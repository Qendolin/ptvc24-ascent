#include "WaterRenderer.h"

#include <glm/glm.hpp>
#include <iostream>

#include "../Camera.h"
#include "../GL/Geometry.h"
#include "../GL/Shader.h"
#include "../GL/StateManager.h"
#include "../GL/Texture.h"
#include "../Game.h"
#include "../Input.h"
#include "../Loader/Loader.h"

WaterRenderer::WaterRenderer() {
    shader = new gl::ShaderPipeline(
        {new gl::ShaderProgram("assets/shaders/waterShader.vert"),
         new gl::ShaderProgram("assets/shaders/waterShader.frag")});
    shader->setDebugLabel("Water_Renderer/shader");

    image = loader::texture("assets/textures/SimpleWater_displace.png", {.srgb = false, .internalFormat = GL_R8});
    width = image->width(), height = image->height();

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    std::vector<float> vertices;
    float yScale = 64.0f / 256.0f, yShift = 16.0f;
    sampler = new gl::Sampler();
    sampler->wrapMode(GL_REPEAT, GL_REPEAT, GL_REPEAT);
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            const float vx = -height / 2.0f + height * i / (float)height;
            const float vz = -width / 2.0f + width * j / (float)width;
            // vertex
            vertices.push_back(vx);  // vx
            vertices.push_back(vz);  // vz
        }
    }
    std::cout << "Loaded " << vertices.size() / 3 << " vertices for water renderer" << std::endl;
    std::vector<unsigned> indices;
    for (unsigned i = 0; i < height - 1; i += rez) {
        for (unsigned j = 0; j < width; j += rez) {
            for (unsigned k = 2; k > 0; k--) {
                indices.push_back(j + width * (i + (k - 1) * rez));
            }
        }
    }

    std::cout << "Loaded " << indices.size() << " indices" << std::endl;

    // first, configure the cube's VAO (and terrainVBO + terrainIBO)
    vao = new gl::VertexArray();
    vao->layout(0, 0, 2, GL_FLOAT, false, 0);

    gl::Buffer *vbo = new gl::Buffer();
    vbo->allocate(vertices.data(), sizeof(float) * vertices.size(), 0);

    gl::Buffer *ibo = new gl::Buffer();
    ibo->allocate(indices.data(), indices.size() * sizeof(unsigned int), 0);
    vao->bindBuffer(0, *vbo, 0, 2 * sizeof(float));
    vao->bindElementBuffer(*ibo);

    vao->own(vbo);
    vao->own(ibo);
}

WaterRenderer::~WaterRenderer() {
    delete shader;
    delete vao;
}

void WaterRenderer::render(Camera &camera) {
    const int numStrips = (height - 1) / rez;
    const int numTrisPerStrip = (width / rez) * 2 - 2;
    std::cout << "Created lattice of " << numStrips << " strips with " << numTrisPerStrip << " triangles each" << std::endl;
    std::cout << "Created " << numStrips * numTrisPerStrip << " triangles total" << std::endl;
    gl::pushDebugGroup("WaterRenderer::render");
    auto settings = Game::get().debugSettings.rendering.water;

    if (settings.wireframe) {
        gl::manager->polygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    vao->bind();
    gl::manager->setEnabled({gl::Capability::DepthTest, gl::Capability::CullFace});
    gl::manager->depthFunc(gl::DepthFunc::GreaterOrEqual);
    gl::manager->depthMask(true);
    gl::manager->cullBack();
    shader->bind();
    image->bind(0);
    sampler->bind(0);
    shader->vertexStage()->setUniform("projectionView", camera.viewProjectionMatrix());
    shader->vertexStage()->setUniform("view", camera.viewMatrix());
    shader->vertexStage()->setUniform("time", (float)Game::get().input->time());
    for (int strip = 0; strip < numStrips; strip++) {
        glDrawElements(GL_TRIANGLE_STRIP,                                            // primitive type
                       numTrisPerStrip + 2,                                          // number of indices to render
                       GL_UNSIGNED_INT,                                              // index data type
                       (void *)(sizeof(unsigned) * (numTrisPerStrip + 2) * strip));  // offset to starting index
    }
    if (settings.wireframe)
        gl::manager->polygonMode(GL_FRONT_AND_BACK, GL_FILL);
    gl::popDebugGroup();
}