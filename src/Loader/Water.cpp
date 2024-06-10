#include "Water.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <stb_image.h>

#include <dds_image/dds.hpp>
#include <vector>

#include "../GL/Geometry.h"
#include "../GL/Texture.h"
#include "../Loader/Loader.h"
#include "../Physics/Physics.h"
#include "../Util/Log.h"

namespace loader {

WaterData::WaterData(WaterData::Files files) {
    height = loader::image(files.height);
    normal = loader::image(files.normal);
}

WaterData::~WaterData() = default;

Water::Water(WaterData &data, float size, float heightScale, glm::vec3 origin, int subdivisions)
    : subdivisions_(subdivisions),
      origin_(origin),
      heightScale_(heightScale) {
    height_ = new gl::Texture(GL_TEXTURE_2D);
    height_->setDebugLabel("Water/height");
    height_->allocate(0, GL_R8, data.height.width, data.height.height);
    height_->load(0, data.height.width, data.height.height, GL_RGBA, GL_UNSIGNED_BYTE, data.height.data.get());
    height_->generateMipmap();
    normal_ = loader::texture(data.height, loader::TextureParameters{.mipmap = true, .srgb = false, .internalFormat = GL_RGB8_SNORM});
    normal_->setDebugLabel("Water/normal");
    struct Vertex {
        glm::vec2 position;
        glm::vec2 uv;
    };

    glm::vec2 dimensions = {size, size};
    // scale to fit
    if (data.height.width > data.height.height)
        dimensions.y *= (float)data.height.height / data.height.width;
    else
        dimensions.x *= (float)data.height.width / data.height.height;

    std::vector<Vertex> vertices;
    glm::vec2 fraction = {1.0 / subdivisions, 1.0 / subdivisions};
    for (int y = 0; y < subdivisions; y++) {
        for (int x = 0; x < subdivisions; x++) {
            vertices.push_back(Vertex{
                .position = dimensions * glm::vec2{x, y} * fraction - dimensions / 2.0f,
                .uv = glm::vec2{x, y} * fraction,
            });
            vertices.push_back(Vertex{
                .position = dimensions * glm::vec2{x, y + 1} * fraction - dimensions / 2.0f,
                .uv = glm::vec2{x, y + 1} * fraction,
            });
            vertices.push_back(Vertex{
                .position = dimensions * glm::vec2{x + 1, y} * fraction - dimensions / 2.0f,
                .uv = glm::vec2{x + 1, y} * fraction,
            });
            vertices.push_back(Vertex{
                .position = dimensions * glm::vec2{x + 1, y + 1} * fraction - dimensions / 2.0f,
                .uv = glm::vec2{x + 1, y + 1} * fraction,
            });
        }
    }

    vao_ = new gl::VertexArray();
    vao_->setDebugLabel("water/vao");
    vao_->layout(0, 0, 2, GL_FLOAT, false, offsetof(Vertex, position));
    vao_->layout(0, 1, 2, GL_FLOAT, false, offsetof(Vertex, uv));

    gl::Buffer *vbo = new gl::Buffer();
    vbo->setDebugLabel("water/vbo");
    vbo->allocate(vertices.data(), sizeof(Vertex) * vertices.size(), 0);

    vao_->bindBuffer(0, *vbo, 0, sizeof(Vertex));
    vao_->own(vbo);  // vao will delete the vbo
}

Water::~Water() {
    delete height_;
    delete vao_;
    delete normal_;
}

}  // namespace loader
