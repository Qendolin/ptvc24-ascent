#include "TerrainRenderer.h"

#include <glm/glm.hpp>
#include <iostream>

#include "../Camera.h"
#include "../GL/Geometry.h"
#include "../GL/Shader.h"
#include "../GL/StateManager.h"
#include "../GL/Texture.h"
#include "../Game.h"
#include "../Loader/Loader.h"

TerrainRenderer::TerrainRenderer() {
    shader = new gl::ShaderPipeline(
        {new gl::ShaderProgram("assets/shaders/terrain.vert"),
         new gl::ShaderProgram("assets/shaders/terrain.frag"),
         new gl::ShaderProgram("assets/shaders/terrain.tesc"),
         new gl::ShaderProgram("assets/shaders/terrain.tese")});
    shader->setDebugLabel("Tessellation_renderer/shader");

    // load and create a texture
    // -------------------------
    sampler = new gl::Sampler();
    // height will be 0 outside of image bounds
    sampler->wrapMode(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER, 0);
    sampler->borderColor(glm::vec4(0.0));
    sampler->filterMode(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);  // mipmap levels aren't selected automatically in the tese shader.

    loader::Image image = loader::image("assets/textures/iceland_heightmap.png");
    int width = image.width, height = image.height;

    heightMap = new gl::Texture(GL_TEXTURE_2D);
    heightMap->allocate(0, GL_R8, width, height);
    heightMap->load(0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, image.data.get());
    // mip levels are for normals
    heightMap->generateMipmap();
    std::cout << "Loaded heightmap of size " << width << " x " << height << std::endl;

    std::vector<float> vertices;
    std::cout << "values of width and height are: " << height << ", " << width << std::endl;
    for (int y = 0; y <= resolution - 1; y++) {
        for (int x = 0; x <= resolution - 1; x++) {
            vertices.push_back(-width / 2.0f + width * x / (float)resolution);    // v.x
            vertices.push_back(0.0f);                                             // v.y
            vertices.push_back(-height / 2.0f + height * y / (float)resolution);  // v.z
            vertices.push_back(x / (float)resolution);                            // u
            vertices.push_back(y / (float)resolution);                            // v

            vertices.push_back(-width / 2.0f + width * x / (float)resolution);          // v.x
            vertices.push_back(0.0f);                                                   // v.y
            vertices.push_back(-height / 2.0f + height * (y + 1) / (float)resolution);  // v.z
            vertices.push_back(x / (float)resolution);                                  // u
            vertices.push_back((y + 1) / (float)resolution);                            // v

            vertices.push_back(-width / 2.0f + width * (x + 1) / (float)resolution);  // v.x
            vertices.push_back(0.0f);                                                 // v.y
            vertices.push_back(-height / 2.0f + height * y / (float)resolution);      // v.z
            vertices.push_back((x + 1) / (float)resolution);                          // u
            vertices.push_back(y / (float)resolution);                                // v

            vertices.push_back(-width / 2.0f + width * (x + 1) / (float)resolution);    // v.x
            vertices.push_back(0.0f);                                                   // v.y
            vertices.push_back(-height / 2.0f + height * (y + 1) / (float)resolution);  // v.z
            vertices.push_back((x + 1) / (float)resolution);                            // u
            vertices.push_back((y + 1) / (float)resolution);                            // v
        }
    }

    std::cout << "Loaded " << resolution * resolution << " patches of 4 control points each" << std::endl;
    std::cout << "Processing " << resolution * resolution * 4 << " vertices in vertex shader" << std::endl;

    vao = new gl::VertexArray();
    vao->layout(0, 0, 3, GL_FLOAT, false, 0);
    vao->layout(0, 1, 2, GL_FLOAT, false, 3 * sizeof(float));

    gl::Buffer *vbo = new gl::Buffer();
    vbo->allocate(vertices.data(), sizeof(float) * vertices.size(), 0);

    vao->bindBuffer(0, *vbo, 0, 5 * sizeof(float));
    vao->own(vbo);  // vao will delete the vbo
}

TerrainRenderer::~TerrainRenderer() {
    delete shader;
    delete sampler;
    delete heightMap;
    delete vao;
}

void TerrainRenderer::render(Camera &camera) {
    gl::pushDebugGroup("TerrainRenderer::render");
    auto settings = Game::get().debugSettings.rendering.terrain;

    if (settings.wireframe)
        gl::manager->polygonMode(GL_FRONT_AND_BACK, GL_LINE);

    vao->bind();
    gl::manager->setEnabled({gl::Capability::DepthTest, gl::Capability::CullFace});
    gl::manager->depthFunc(gl::DepthFunc::GreaterOrEqual);
    gl::manager->depthMask(true);
    gl::manager->cullBack();
    shader->bind();

    heightMap->bind(0);
    sampler->bind(0);

    glm::vec3 origin = camera.position;
    if (settings.fixedLodOrigin) {
        origin = glm::vec3(0.0);
    }
    shader->get(GL_TESS_CONTROL_SHADER)->setUniform("u_camera_pos", origin);

    shader->get(GL_TESS_EVALUATION_SHADER)->setUniform("u_view_projection_mat", camera.viewProjectionMatrix());
    shader->get(GL_TESS_EVALUATION_SHADER)->setUniform("u_height_scale", settings.heightScale);

    glPatchParameteri(GL_PATCH_VERTICES, 4);
    glDrawArrays(GL_PATCHES, 0, 4 * resolution * resolution);

    if (settings.wireframe)
        gl::manager->polygonMode(GL_FRONT_AND_BACK, GL_FILL);
    gl::popDebugGroup();
}