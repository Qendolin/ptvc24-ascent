#include "TerrainRenderer.h"

#include <stb_image.h>

#include <glm/glm.hpp>
#include <iostream>

#include "../Camera.h"
#include "../GL/Geometry.h"
#include "../GL/Shader.h"
#include "../GL/StateManager.h"
#include "../GL/Texture.h"
#include "../Game.h"
#include "../Loader/Loader.h"
#include "../Util/Log.h"
#include "IblEnvironment.h"

TerrainRenderer::TerrainRenderer() {
    shader = new gl::ShaderPipeline(
        {new gl::ShaderProgram("assets/shaders/terrain.vert"),
         new gl::ShaderProgram("assets/shaders/terrain.frag"),
         new gl::ShaderProgram("assets/shaders/terrain.tesc"),
         new gl::ShaderProgram("assets/shaders/terrain.tese")});
    shader->setDebugLabel("terrain_renderer/shader");

    // load and create a texture
    // -------------------------
    sampler = new gl::Sampler();
    sampler->setDebugLabel("terrain_renderer/height_sampler");
    sampler->wrapMode(GL_MIRRORED_REPEAT, GL_MIRRORED_REPEAT, 0);
    sampler->filterMode(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);  // mipmap levels aren't selected automatically in the tese shader.

    std::vector<uint8_t> image_raw = loader::binary("assets/textures/terrain_height.png");
    int w, h, ch;
    auto image = stbi_load_16_from_memory(image_raw.data(), image_raw.size(), &w, &h, &ch, 1);
    if (image == nullptr) {
        PANIC("Error loading 16pbc grayscale heightmap image");
    }

    height = new gl::Texture(GL_TEXTURE_2D);
    height->setDebugLabel("terrain_renderer/height_tex");
    height->allocate(1, GL_R16, w, h);
    height->load(0, w, h, GL_RED, GL_UNSIGNED_SHORT, image);
    delete image;
    LOG_DEBUG("Loaded heightmap of size " << w << " x " << h);

    albedo = loader::texture("assets/textures/terrain_albedo.jpg", loader::TextureParameters{.mipmap = true, .srgb = true, .internalFormat = GL_SRGB8});
    albedo->setDebugLabel("terrain_renderer/albedo_tex");

    normal = loader::texture("assets/textures/terrain_normal.png", loader::TextureParameters{.mipmap = true, .srgb = false, .internalFormat = GL_RGB8_SNORM});
    normal->setDebugLabel("terrain_renderer/normal_tex");

    occlusion = loader::texture("assets/textures/terrain_ao.png", loader::TextureParameters{.mipmap = true, .srgb = false, .internalFormat = GL_R8});
    occlusion->setDebugLabel("terrain_renderer/ao_tex");

    std::vector<float> vertices;
    for (int y = 0; y <= resolution - 1; y++) {
        for (int x = 0; x <= resolution - 1; x++) {
            vertices.push_back(-w / 2.0f + w * x / (float)resolution);  // v.x
            vertices.push_back(0.0f);                                   // v.y
            vertices.push_back(-h / 2.0f + h * y / (float)resolution);  // v.z
            vertices.push_back(x / (float)resolution);                  // u
            vertices.push_back(y / (float)resolution);                  // v

            vertices.push_back(-w / 2.0f + w * x / (float)resolution);        // v.x
            vertices.push_back(0.0f);                                         // v.y
            vertices.push_back(-h / 2.0f + h * (y + 1) / (float)resolution);  // v.z
            vertices.push_back(x / (float)resolution);                        // u
            vertices.push_back((y + 1) / (float)resolution);                  // v

            vertices.push_back(-w / 2.0f + w * (x + 1) / (float)resolution);  // v.x
            vertices.push_back(0.0f);                                         // v.y
            vertices.push_back(-h / 2.0f + h * y / (float)resolution);        // v.z
            vertices.push_back((x + 1) / (float)resolution);                  // u
            vertices.push_back(y / (float)resolution);                        // v

            vertices.push_back(-w / 2.0f + w * (x + 1) / (float)resolution);  // v.x
            vertices.push_back(0.0f);                                         // v.y
            vertices.push_back(-h / 2.0f + h * (y + 1) / (float)resolution);  // v.z
            vertices.push_back((x + 1) / (float)resolution);                  // u
            vertices.push_back((y + 1) / (float)resolution);                  // v
        }
    }

    vao = new gl::VertexArray();
    vao->setDebugLabel("terrain_renderer/vao");
    vao->layout(0, 0, 3, GL_FLOAT, false, 0);
    vao->layout(0, 1, 2, GL_FLOAT, false, 3 * sizeof(float));

    gl::Buffer *vbo = new gl::Buffer();
    vbo->setDebugLabel("terrain_renderer/vbo");
    vbo->allocate(vertices.data(), sizeof(float) * vertices.size(), 0);

    vao->bindBuffer(0, *vbo, 0, 5 * sizeof(float));
    vao->own(vbo);  // vao will delete the vbo
}

TerrainRenderer::~TerrainRenderer() {
    delete shader;
    delete sampler;
    delete height;
    delete normal;
    delete albedo;
    delete occlusion;
    delete vao;
}

void TerrainRenderer::render(Camera &camera, IblEnvironment &env) {
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

    height->bind(0);
    sampler->bind(0);
    albedo->bind(1);
    sampler->bind(1);
    normal->bind(2);
    sampler->bind(2);
    occlusion->bind(3);
    sampler->bind(3);

    env.diffuse().bind(4);
    env.cubemapSampler().bind(4);
    env.specular().bind(5);
    env.cubemapSampler().bind(5);
    env.brdfLut().bind(6);
    env.lutSampler().bind(6);

    glm::vec3 origin = camera.position;
    if (settings.fixedLodOrigin) {
        origin = glm::vec3(0.0);
    }
    shader->get(GL_TESS_CONTROL_SHADER)->setUniform("u_camera_pos", origin);

    shader->get(GL_TESS_EVALUATION_SHADER)->setUniform("u_view_projection_mat", camera.viewProjectionMatrix());
    shader->get(GL_TESS_EVALUATION_SHADER)->setUniform("u_height_scale", settings.heightScale);

    shader->fragmentStage()->setUniform("u_view_mat", camera.viewMatrix());
    shader->fragmentStage()->setUniform("u_camera_pos", camera.position);

    glPatchParameteri(GL_PATCH_VERTICES, 4);
    glDrawArrays(GL_PATCHES, 0, 4 * resolution * resolution);

    if (settings.wireframe)
        gl::manager->polygonMode(GL_FRONT_AND_BACK, GL_FILL);
    gl::popDebugGroup();
}