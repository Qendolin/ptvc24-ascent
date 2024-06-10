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

struct WaterData {
    struct Files {
        std::string height;
        std::string normal;
    };

    loader::Image height;
    loader::Image normal;

    WaterData(Files files);
    ~WaterData();
};

class Water {
   private:
    gl::Texture* height_;
    gl::Texture* normal_;
    gl::VertexArray* vao_;
    int subdivisions_;
    glm::vec3 origin_;
    float heightScale_;

   public:
    Water(WaterData& data, float size, float heightScale, glm::vec3 origin, int subdivisions);
    ~Water();

    gl::VertexArray& meshVao() {
        return *vao_;
    }

    gl::Texture& heightTexture() {
        return *height_;
    }

    gl::Texture& normalTexture() {
        return *normal_;
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
};
}  // namespace loader