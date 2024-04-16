#include "SkyRenderer.h"

#include "../Camera.h"
#include "../GL/Geometry.h"
#include "../GL/Shader.h"
#include "../GL/StateManager.h"
#include "../GL/Texture.h"
#include "../Loader/Environment.h"

SkyRenderer::SkyRenderer() {
    shader = new gl::ShaderPipeline(
        {new gl::ShaderProgram("assets/shaders/sky.vert"),
         new gl::ShaderProgram("assets/shaders/sky_cubemap.frag")});
    shader->setDebugLabel("sky_renderer/shader");

    gl::Buffer *vbo = new gl::Buffer();
    vbo->setDebugLabel("sky_renderer/vbo");
    glm::vec2 quad_verts[] = {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}};
    vbo->allocate(&quad_verts, sizeof(quad_verts), 0);

    quad = new gl::VertexArray();
    quad->setDebugLabel("sky_renderer/vao");
    quad->layout(0, 0, 2, GL_FLOAT, false, 0);
    quad->bindBuffer(0, *vbo, 0, 2 * 4);
    quad->own(vbo);

    sampler = new gl::Sampler();
    sampler->setDebugLabel("sky_renderer/sampler");
    sampler->wrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    sampler->filterMode(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

    // TODO: dont hardcore the names
    loader::IblEnv *env_sky = loader::environment("assets/textures/skybox/kloofendal.iblenv");
    cubemap = new gl::Texture(GL_TEXTURE_CUBE_MAP);
    cubemap->setDebugLabel("sky_renderer/cubemap");
    cubemap->allocate(1, GL_RGB16F, env_sky->baseSize, env_sky->baseSize);
    cubemap->load(0, env_sky->baseSize, env_sky->baseSize, 6, GL_RGB, GL_FLOAT, env_sky->all().data());
    delete env_sky;
}

SkyRenderer::~SkyRenderer() {
    delete shader;
    delete quad;
    delete sampler;
    delete cubemap;
}

void SkyRenderer::render(Camera &camera) {
    gl::pushDebugGroup("SkyRenderer::render");

    gl::manager->setEnabled({gl::Capability::DepthTest, gl::Capability::DepthClamp});
    gl::manager->depthFunc(gl::DepthFunc::GreaterOrEqual);

    quad->bind();
    shader->bind();

    sampler->bind(0);
    cubemap->bind(0);

    shader->fragmentStage()->setUniform("u_view_mat", camera.viewMatrix());
    shader->fragmentStage()->setUniform("u_projection_mat", camera.projectionMatrix());

    // The sky is rendered using a single, full-screen quad
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    gl::popDebugGroup();
}