#include "Graphics.h"

namespace gltf = tinygltf;

namespace loader {

gl::Texture *loadTexture(GraphicsLoadingContext &context, const gltf::TextureInfo &texture_info, GLenum internalFormat) {
    if (texture_info.index < 0) {
        return nullptr;
    }
    if (texture_info.texCoord != 0) {
        LOG("only texCoord=0 is supported");
        return nullptr;
    }
    const gltf::Texture &texture = context.model.textures[texture_info.index];
    const gltf::Image &image = context.model.images[texture.source];
    if (image.bits != 8) {
        LOG("only 8-bit images are supported");
        return nullptr;
    }

    GLenum format;
    if (image.component == 1) {
        format = GL_RED;
    } else if (image.component == 2) {
        format = GL_RG;
    } else if (image.component == 3) {
        format = GL_RGB;
    } else if (image.component == 4) {
        format = GL_RGBA;
    }

    gl::Texture *result = new gl::Texture(GL_TEXTURE_2D);
    result->allocate(0, internalFormat, image.width, image.height, 1);
    result->load(0, image.width, image.height, 1, format, GL_UNSIGNED_BYTE, image.image.data());
    result->generateMipmap();
    return result;
}

Material &loadMaterial(GraphicsLoadingContext &context, const gltf::Material &material) {
    Material &result = context.newMaterial();
    result.name = material.name;
    result.albedoFactor = glm::make_vec4(&material.pbrMetallicRoughness.baseColorFactor[0]);
    result.metallicRoughnessFactor = {
        material.pbrMetallicRoughness.metallicFactor,
        material.pbrMetallicRoughness.roughnessFactor,
    };

    result.albedo = loadTexture(context, material.pbrMetallicRoughness.baseColorTexture, GL_SRGB8_ALPHA8);
    if (result.albedo != nullptr) result.albedo->setDebugLabel("gltf/texture/albedo");
    result.occlusionMetallicRoughness = loadTexture(context, material.pbrMetallicRoughness.metallicRoughnessTexture, GL_RGB8);
    if (result.occlusionMetallicRoughness != nullptr) result.occlusionMetallicRoughness->setDebugLabel("gltf/texture/orm");
    gltf::TextureInfo normal_info = {};
    normal_info.index = material.normalTexture.index;
    normal_info.texCoord = material.normalTexture.texCoord;
    result.normal = loadTexture(context, normal_info, GL_RGB8);
    if (result.normal != nullptr) result.normal->setDebugLabel("gltf/texture/normal");

    return result;
}

Material &loadDefaultMaterial(GraphicsLoadingContext &context) {
    Material &result = context.newMaterial();
    result.name = "Default";
    result.albedoFactor = glm::vec4(1.0);
    result.metallicRoughnessFactor = glm::vec2(0.0, 1.0);
    result.albedo = loader::texture("assets/textures/default_albedo.png", {.srgb = true});
    result.albedo->setDebugLabel("gltf/texture/default_albedo");
    result.occlusionMetallicRoughness = loader::texture("assets/textures/default_orm.png");
    result.occlusionMetallicRoughness->setDebugLabel("gltf/texture/default_orm");
    result.normal = loader::texture("assets/textures/default_normal.png");
    result.normal->setDebugLabel("gltf/texture/default_normal");

    context.defaultMaterial = context.materials.size() - 1;

    return result;
}

}  // namespace loader