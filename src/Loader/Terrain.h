#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>

#include "Loader.h"

#pragma region ForwardDecl
#include "../GL/Declarations.h"
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

    loader::Image albedo;
    loader::Image occlusion;
    loader::Image normal;
    loader::TerrainHeightmap height;

    TerrainData(Files files);
};

class Terrain {
   private:
    gl::Texture* height_;
    gl::Texture* albedo_;
    gl::Texture* normal_;
    gl::Texture* occlusion_;
    gl::VertexArray* vao_;
    int subdivisions_;

   public:
    glm::vec3 position = glm::vec3{0.0, 0.0, 0.0};
    float height = 1.0;

    Terrain(TerrainData& data, float size, int subdivisions);
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
};
}  // namespace loader