#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "Object.h"

namespace GL {

// References:
// https://www.khronos.org/opengl/wiki/Texture
class Texture : public GLObject {
   private:
    GLuint id_ = 0;
    GLenum type_ = 0;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    uint32_t depth_ = 0;

    Texture(GLenum type, GLuint id) : type_(type), id_(id) {
    }

    ~Texture() {
        checkDestroyed(GL_TEXTURE);
    }

   public:
    // creat a texture with the given type. Usually `GL_TEXTURE_2D`.
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glCreateTextures.xhtml)
    Texture(GLenum type);

    // Create an alias to this texture with the given type.
    // Useful for 2D array textures or cubemaps.
    Texture* as(GLenum type);

    void setDebugLabel(const std::string& label);

    GLenum type() const;

    // @returns `1`, `2` or `3` depending on the type
    int dimensions() const;

    GLuint id() const;

    uint32_t width() const;

    uint32_t height() const;

    uint32_t depth() const;

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBindTextureUnit.xhtml)
    void bind(int unit);

    void destroy();

    // References:
    // - [Wiki](https://www.khronos.org/opengl/wiki/Texture_Storage#Texture_views)
    // - [glTextureView](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTextureView.xhtml)
    Texture* createView(GLenum type, GLenum internal_format, int min_level, int max_mevel, int min_layer, int max_layer);

    /**
     * Allocate space for the texture.
     * [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexStorage3D.xhtml)
     *
     * @param levels the number of mipmap levels. Use `1` to disable mipmapping. Use `0` to enable all levels.
     * @param internal_format usually `GL_RGB8` or `GL_RGBA8`. See [here](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexStorage2D.xhtml) for all.
     * @param width width in pixels
     * @param height height in pixels or number of layers for 1D array textures
     * @param depth depth in pixels or number of layers for 2D array textures. Use `6` for cubemaps.
     */
    void allocate(GLint levels, GLenum internal_format, uint32_t width, uint32_t height, uint32_t depth);

    /**
     * Allocate space for the texture.
     * [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexStorage2D.xhtml)
     *
     * @param levels the number of mipmap levels. Use `1` to disable mipmapping. Use `0` to enable all levels.
     * @param internal_format usually `GL_RGB8` or `GL_RGBA8`. See [here](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexStorage2D.xhtml) for all.
     * @param width width in pixels
     * @param height height in pixels or number of layers for 1D array textures
     */
    void allocate(GLint levels, GLenum internal_format, uint32_t width, uint32_t height) {
        allocate(levels, internal_format, width, height, 1);
    }

    /**
     * Allocate space for the texture.
     * [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexStorage1D.xhtml)
     *
     * @param levels the number of mipmap levels. Use `1` to disable mipmapping. Use `0` to enable all levels.
     * @param internal_format usually `GL_RGB8` or `GL_RGBA8`. See [here](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexStorage2D.xhtml) for all.
     * @param width width in pixels
     */
    void allocate(GLint levels, GLenum internal_format, uint32_t width) {
        allocate(levels, internal_format, width, 1, 1);
    }

    void allocateMS(GLenum internal_format, uint32_t width, uint32_t height, uint32_t depth, int samples, bool fixed_sample_locations);

    void allocateMS(GLenum internal_format, uint32_t width, uint32_t height, int samples, bool fixed_sample_locations) {
        allocateMS(internal_format, width, height, 1, samples, fixed_sample_locations);
    }

    /**
     * Load data into the texture. Must be allocated first!
     * [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexSubImage3D.xhtml)
     *
     * @param level the mipmap level to load data into. Usually `0`.
     * @param width the width of the region that will be written to. Usually the same as was allocated.
     * @param height the height of the region that will be written to. Usually the same as was allocated.
     * @param depth the depth of the region that will be written to. Usually the same as was allocated.
     * @param format the components of the data. One of `GL_RED`, `GL_RG`, `GL_RGB`, `GL_BGR`, `GL_RGBA`, `GL_DEPTH_COMPONENT`, or `GL_STENCIL_INDEX`.
     * @param type the data type of the pixel values. Usually `GL_UNSIGNED_BYTE` or sometimes `GL_FLOAT`.
     * @param data pointer to the data.
     */
    void load(int level, uint32_t width, uint32_t height, uint32_t depth, GLenum format, GLenum type, const void* data);

    /**
     * Load data into the texture. Must be allocated first!
     * [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexSubImage2D.xhtml)
     *
     * @param level the mipmap level to load data into. Usually `0`.
     * @param width the width of the region that will be written to. Usually the same as was allocated.
     * @param height the height of the region that will be written to. Usually the same as was allocated.
     * @param format the components of the data. One of `GL_RED`, `GL_RG`, `GL_RGB`, `GL_BGR`, `GL_RGBA`, `GL_DEPTH_COMPONENT`, or `GL_STENCIL_INDEX`.
     * @param type the data type of the pixel values. Usually `GL_UNSIGNED_BYTE` or sometimes `GL_FLOAT`.
     * @param data pointer to the data.
     */
    void load(int level, uint32_t width, uint32_t height, GLenum format, GLenum type, const void* data) {
        load(level, width, height, 1, format, type, data);
    }

    /**
     * Load data into the texture. Must be allocated first!
     * [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexSubImage1D.xhtml)
     *
     * @param level the mipmap level to load data into. Usually `0`.
     * @param width the width of the region that will be written to. Usually the same as was allocated.
     * @param format the components of the data. One of `GL_RED`, `GL_RG`, `GL_RGB`, `GL_BGR`, `GL_RGBA`, `GL_DEPTH_COMPONENT`, or `GL_STENCIL_INDEX`.
     * @param type the data type of the pixel values. Usually `GL_UNSIGNED_BYTE` or sometimes `GL_FLOAT`.
     * @param data pointer to the data.
     */
    void load(int level, uint32_t width, GLenum format, GLenum type, const void* data) {
        load(level, width, 1, 1, format, type, data);
    }

    /**
     * Generates the mipmap levels `base + 1` to `max` from the data in `base`.
     * See `mipmapLevels`
     *
     * [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGenerateMipmap.xhtml)
     */
    void generateMipmap();

    // References:
    // - [Wiki](https://www.khronos.org/opengl/wiki/Texture#Mipmap_range)
    // - [glTexParameter](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexParameter.xhtml)
    void mipmapLevels(int base, int max);

    /**
     * [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexParameter.xhtml)
     *
     * @param mode `GL_DEPTH_COMPONENT` or `GL_STENCIL_INDEX`
     */
    void depthStencilTextureMode(GLenum mode);
};

// References:
// https://www.khronos.org/opengl/wiki/Sampler_Object
class Sampler : public GLObject {
   private:
    GLuint id_ = 0;

    ~Sampler() {
        checkDestroyed(GL_SAMPLER);
    }

   public:
    Sampler();

    void destroy();

    GLuint id() const;

    void setDebugLabel(const std::string& label);

    void bind(int unit) const;

    /**
     * [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glSamplerParameter.xhtml)
     *
     * @param min Minifiacation filter. One of `GL_NEAREST`, `GL_LINEAR` or `GL_{ NEAREST | LINEAR }_MIPMAP_{ NEAREST | LINEAR }`
     * @param mag Magnification filter. Either `GL_NEAREST` or `GL_LINEAR`
     */
    void filterMode(GLenum min, GLenum mag);

    /**
     * [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glSamplerParameter.xhtml)
     *
     * @param s The wrapping mode for the `s`/`u`/`x` axis. One of `GL_CLAMP_TO_EDGE`, `GL_MIRRORED_REPEAT`, `GL_REPEAT`, or `GL_MIRROR_CLAMP_TO_EDGE`.
     * @param t The wrapping mode for the `t`/`v`/`y` axis. One of `GL_CLAMP_TO_EDGE`, `GL_MIRRORED_REPEAT`, `GL_REPEAT`, or `GL_MIRROR_CLAMP_TO_EDGE`.
     * @param r The wrapping mode for the `r`/`w`/`z` axis. One of `GL_CLAMP_TO_EDGE`, `GL_MIRRORED_REPEAT`, `GL_REPEAT`, or `GL_MIRROR_CLAMP_TO_EDGE`.
     */
    void wrapMode(GLenum s, GLenum t, GLenum r);

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glSamplerParameter.xhtml)
    void compareMode(GLenum mode, GLenum fn);

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glSamplerParameter.xhtml)
    void borderColor(glm::vec4 color);

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glSamplerParameter.xhtml)
    void anisotropicFilter(float quality);

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glSamplerParameter.xhtml)
    void lodBias(float bias);
};

}  // namespace GL
