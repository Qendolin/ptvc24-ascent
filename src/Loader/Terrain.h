#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>

#include "Loader.h"

#pragma region ForwardDecl
#include "../GL/Declarations.h"
namespace JPH {
class HeightFieldShapeSettings;
class BodyCreationSettings;
class BodyInterface;
class Body;
}  // namespace JPH
namespace dds {
struct Image;
}
#pragma endregion

namespace loader {

struct TerrainHeightmap {
    int width = 0;
    int height = 0;
    std::shared_ptr<uint16_t> data;
};

struct TerrainData {
    struct Files {
        std::string albedo;
        std::string height;
        std::string occlusion;
        std::string normal;
    };

    std::shared_ptr<dds::Image> albedo;
    loader::Image occlusion;
    loader::Image normal;
    loader::TerrainHeightmap height;

    TerrainData(Files files);
    ~TerrainData();
};

class Terrain {
   private:
    gl::Texture* height_;
    gl::Texture* albedo_;
    gl::Texture* normal_;
    gl::Texture* occlusion_;
    gl::VertexArray* vao_;
    int subdivisions_;
    glm::vec3 origin_;
    float heightScale_;

    JPH::HeightFieldShapeSettings* heightFieldShape_;
    std::unique_ptr<JPH::Body> body_;

   public:
    Terrain(TerrainData& data, float size, float heightScale, glm::vec3 origin, int subdivisions);
    ~Terrain();

    gl::VertexArray& meshVao() {
        return *vao_;
    }

    gl::Texture& heightTexture() {
        return *height_;
    }

    gl::Texture& albedoTexture() {
        return *albedo_;
    }

    gl::Texture& normalTexture() {
        return *normal_;
    }

    gl::Texture& occlusionTexture() {
        return *occlusion_;
    }

    size_t patchCount() {
        return 4 * subdivisions_ * subdivisions_;
    }

    glm::vec3 origin() {
        return origin_;
    }

    float heightScale() {
        return heightScale_;
    }

    void createPhysicsBody(JPH::BodyInterface& physics);

    void destroyPhysicsBody(JPH::BodyInterface& physics);

    JPH::Body* physicsBody() {
        return body_.get();
    }
};
}  // namespace loader