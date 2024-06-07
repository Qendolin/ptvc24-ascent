#include "Terrain.h"

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

TerrainData::TerrainData(TerrainData::Files files) {
    std::vector<uint8_t> height_raw = loader::binary(files.height);
    int w, h, ch;
    auto height_pixels = stbi_load_16_from_memory(height_raw.data(), height_raw.size(), &w, &h, &ch, 1);
    if (height_pixels == nullptr) {
        PANIC("Error loading 16pbc grayscale heightmap image");
    }
    height = TerrainHeightmap{
        .width = w,
        .height = h,
        .data = std::shared_ptr<uint16_t>(height_pixels),
    };

    dds::Image *albedo_dds = new dds::Image();
    albedo_dds->data = loader::binary(files.albedo);
    auto dds_read_result = dds::readImage(albedo_dds->data.data(), albedo_dds->data.size(), albedo_dds);
    if (dds_read_result != dds::ReadResult::Success) {
        PANIC("Failed to read terrain heightmap dds");
    }
    albedo = std::shared_ptr<dds::Image>(albedo_dds);
    normal = loader::image(files.normal);
    occlusion = loader::image(files.occlusion);
}

TerrainData::~TerrainData() = default;

Terrain::Terrain(TerrainData &data, float size, float heightScale, glm::vec3 origin, int subdivisions)
    : subdivisions_(subdivisions),
      origin_(origin),
      heightScale_(heightScale) {
    height_ = new gl::Texture(GL_TEXTURE_2D);
    height_->setDebugLabel("terrain/height");
    height_->allocate(0, GL_R16, data.height.width, data.height.height);
    height_->load(0, data.height.width, data.height.height, GL_RED, GL_UNSIGNED_SHORT, data.height.data.get());
    height_->generateMipmap();

    albedo_ = new gl::Texture(GL_TEXTURE_2D);
    albedo_->setDebugLabel("terrain/albedo");
    uint32_t albedo_mips = std::min(static_cast<uint32_t>(std::log2(std::max(data.albedo->width, data.albedo->height))), data.albedo->numMips);
    albedo_->allocate(albedo_mips, GL_COMPRESSED_SRGB_S3TC_DXT1_EXT, data.albedo->width, data.albedo->height);

    for (uint32_t mip = 0; mip < albedo_mips; mip++) {
        auto mip_data = data.albedo->mipmaps[mip];
        int width = data.albedo->width >> mip;
        int height = data.albedo->height >> mip;

        albedo_->loadCompressed(mip, width, height, GL_COMPRESSED_SRGB_S3TC_DXT1_EXT, mip_data.size_bytes(), mip_data.data());
    }

    normal_ = loader::texture(data.normal, loader::TextureParameters{.mipmap = true, .srgb = false, .internalFormat = GL_RGB8_SNORM});
    normal_->setDebugLabel("terrain/normal");

    occlusion_ = loader::texture(data.occlusion, loader::TextureParameters{.mipmap = true, .srgb = false, .internalFormat = GL_R8});
    occlusion_->setDebugLabel("terrain/occlusion");

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

    const int collision_subsample = 16;
    std::vector<float> collision_samples;
    int collision_samples_count = std::max(data.height.width / collision_subsample, data.height.height / collision_subsample);
    // jolt heightmap collision must be square
    collision_samples.reserve(collision_samples_count * collision_samples_count);
    for (int y = 0; y < collision_samples_count; y++) {
        for (int x = 0; x < collision_samples_count; x++) {
            float height = JPH::HeightFieldShapeConstants::cNoCollisionValue;
            int ix = x * collision_subsample, iy = y * collision_subsample;

            if (ix < data.height.width || iy < data.height.height) {
                int index = ix + iy * data.height.width;
                // Warning: OOB access may be possible, this is not secure at all!
                uint16_t sample = *(data.height.data.get() + index);
                height = (float)sample / 0xffff;
            }

            collision_samples.push_back(height);
        }
    }

    JPH::Vec3 collision_scale = {JPH::Vec3(dimensions.x / collision_samples_count, heightScale, dimensions.y / collision_samples_count)};
    heightFieldShape_ = new JPH::HeightFieldShapeSettings(
        collision_samples.data(),
        JPH::Vec3{-dimensions.x / 2.0f, 0, -dimensions.y / 2.0f},
        collision_scale,
        collision_samples_count);

    vao_ = new gl::VertexArray();
    vao_->setDebugLabel("terrain/vao");
    vao_->layout(0, 0, 2, GL_FLOAT, false, offsetof(Vertex, position));
    vao_->layout(0, 1, 2, GL_FLOAT, false, offsetof(Vertex, uv));

    gl::Buffer *vbo = new gl::Buffer();
    vbo->setDebugLabel("terrain/vbo");
    vbo->allocate(vertices.data(), sizeof(Vertex) * vertices.size(), 0);

    vao_->bindBuffer(0, *vbo, 0, sizeof(Vertex));
    vao_->own(vbo);  // vao will delete the vbo
}

Terrain::~Terrain() {
    delete height_;
    delete albedo_;
    delete normal_;
    delete occlusion_;
    delete vao_;
    // heightFieldShape_ is already deleted automatically by jolt?
}

void Terrain::createPhysicsBody(JPH::BodyInterface &physics, glm::vec3 offset) {
    auto settings = JPH::BodyCreationSettings(heightFieldShape_, ph::convert(origin_ + offset), JPH::Quat::sIdentity(), JPH::EMotionType::Static, ph::Layers::NON_MOVING);
    body_ = std::unique_ptr<JPH::Body>(physics.CreateBody(settings));
}

void Terrain::destroyPhysicsBody(JPH::BodyInterface &physics) {
    physics.DestroyBody(body_->GetID());
    body_.release();
}

}  // namespace loader
