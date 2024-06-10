#pragma once

#include <array>
#include <glm/glm.hpp>

#include "../Camera.h"
#include "../GL/Declarations.h"

// References:
// https://developer.nvidia.com/gpugems/gpugems/part-ii-lighting-and-shadows/chapter-11-shadow-map-antialiasing
// https://web.archive.org/web/20160602232409if_/http://www.dissidentlogic.com/old/images/NormalOffsetShadows/GDC_Poster_NormalOffset.png

#pragma region ForwardDecl
class Camera;
namespace loader {
class GraphicsData;
class Terrain;
}  // namespace loader
#pragma endregion

class ShadowCaster {
   protected:
    gl::Framebuffer* shadowMap_ = nullptr;
    gl::Texture* depthTexture_ = nullptr;
    uint32_t resolution_;
    glm::mat4 projectionMatrix_;
    glm::mat4 viewMatrix_;

   public:
    virtual ~ShadowCaster();

    virtual void bind();

    gl::Texture* depthTexture() const;

    uint32_t resolution() const {
        return resolution_;
    }

    glm::mat4 viewMatrix() const {
        return viewMatrix_;
    }

    glm::mat4 projectionMatrix() const {
        return projectionMatrix_;
    }

    virtual void debugDraw() = 0;

    void lookAt(glm::vec3 target, glm::vec3 direction, float distance, glm::vec3 up = glm::vec3{0, 1, 0});

    void lookAt(glm::vec3 target, float azimuth, float elevation, float distance, glm::vec3 up = glm::vec3{0, 1, 0});
};

class OrthoShadowCaster : public ShadowCaster {
   public:
    OrthoShadowCaster(int resolution, float extends, float near_plane, float far_plane);

    virtual ~OrthoShadowCaster() = default;

    void debugDraw() override;
};

class CSMShadowCaster : public ShadowCaster {
   public:
    float splitDistance;

    CSMShadowCaster();

    virtual ~CSMShadowCaster() = default;

    void debugDraw() override;

    void setProjectionMatrix(glm::mat4 projection_matrix) {
        this->projectionMatrix_ = projection_matrix;
    }

    void setViewMatrix(glm::mat4 view_matrix) {
        this->viewMatrix_ = view_matrix;
    }

    void setDepthTexture(gl::Texture* depth_texture, int resolution) {
        this->depthTexture_ = depth_texture;
        this->resolution_ = resolution;
    }

    void setFramebuffer(gl::Framebuffer* framebuffer) {
        this->shadowMap_ = framebuffer;
    }

    void bind() override;
};

// References:
// https://ahbejarano.gitbook.io/lwjglgamedev/chapter-17
// https://learnopengl.com/Guest-Articles/2021/CSM
// https://alextardif.com/shadowmapping.html
class CSM {
   public:
    static inline constexpr int CASCADE_COUNT = 4;

   private:
    std::array<CSMShadowCaster, CASCADE_COUNT> cascades_;

    gl::Framebuffer* shadowMap_;
    gl::Texture* depthTexture_;

    float updateInterval_;
    float lastUpdateElapsed_ = 0.0f;

   public:
    CSM(int resolution, float update_interval);
    ~CSM();

    CSMShadowCaster* cascade(int index) {
        return &cascades_[index];
    }

    gl::Texture* depthTexture() {
        return depthTexture_;
    }

    /**
     * Update the shadow caster frustums using the update interval.
     * @returns true when an update happened
     */
    bool update(Camera& camera, glm::vec3 light_dir, float time_delta);

    void bind();
};

class ShadowMapRenderer {
   private:
    gl::ShaderPipeline* objectShader;
    gl::ShaderPipeline* terrainShader;
    gl::Sampler* terrainSampler;

   public:
    ShadowMapRenderer();
    ~ShadowMapRenderer();

    void render(CSM& csm, Camera& camera, loader::GraphicsData& graphics, loader::Terrain& terrain);
};