#include <glm/glm.hpp>

#include "../GL/Declarations.h"

namespace loader {
class GraphicsData;
}  // namespace loader

class ShadowCaster {
   protected:
    gl::Framebuffer* shadowMap_;
    uint32_t resolution_;
    glm::mat4 projectionMatrix_;
    glm::mat4 viewMatrix_;

   public:
    virtual ~ShadowCaster();

    void bind();

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

class ShadowMapRenderer {
   private:
    gl::ShaderPipeline* shader;

   public:
    ShadowMapRenderer();
    ~ShadowMapRenderer();

    void render(ShadowCaster& caster, loader::GraphicsData& graphics);
};