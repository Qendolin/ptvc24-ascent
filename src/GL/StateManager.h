#define GLEW_STATIC
#include <GL/glew.h>

#include <array>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "../Util/Log.h"

#pragma once

namespace gl {

// Features that can be enabled.
// These are not all that are available.
// [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glEnable.xhtml).
enum class Capability : GLenum {
    DepthTest = GL_DEPTH_TEST,
    Blend = GL_BLEND,
    StencilTest = GL_STENCIL_TEST,
    ScissorTest = GL_SCISSOR_TEST,
    CullFace = GL_CULL_FACE,
    DepthClamp = GL_DEPTH_CLAMP,
    PolygonOffsetFill = GL_POLYGON_OFFSET_FILL
};

// [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBlendFunc.xhtml).
enum class BlendFactor : GLenum {
    Zero = GL_ZERO,
    One = GL_ONE,
    SrcColor = GL_SRC_COLOR,
    OneMinusSrcColor = GL_ONE_MINUS_SRC_COLOR,
    DstColor = GL_DST_COLOR,
    OneMinusDstColor = GL_ONE_MINUS_DST_COLOR,
    SrcAlpha = GL_SRC_ALPHA,
    OneMinusSrcAlpha = GL_ONE_MINUS_SRC_ALPHA,
    DstAlpha = GL_DST_ALPHA,
    OneMinusDstAlpha = GL_ONE_MINUS_DST_ALPHA,
    ConstantColor = GL_CONSTANT_COLOR,
    ConstantAlpha = GL_CONSTANT_ALPHA,
    OneMinusConstantAlpha = GL_ONE_MINUS_CONSTANT_ALPHA,
    SrcAlphaSaturate = GL_SRC_ALPHA_SATURATE,
    SrcOneColor = GL_SRC1_COLOR,
    OneMinusSrcOneColor = GL_ONE_MINUS_SRC1_COLOR,
    SrcOneAlpha = GL_SRC1_ALPHA,
    OneMinusSrcOneAlpha = GL_ONE_MINUS_SRC1_ALPHA
};

// [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBlendEquation.xhtml)
enum class BlendEquation : GLenum {
    FuncAdd = GL_FUNC_ADD,
    FuncSubtract = GL_FUNC_SUBTRACT,
    FuncReverseSubtract = GL_FUNC_REVERSE_SUBTRACT,
    Min = GL_MIN,
    Max = GL_MAX
};

// [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glStencilFunc.xhtml)
enum class StencilFunc : GLenum {
    Never = GL_NEVER,
    Less = GL_LESS,
    LessOrEqual = GL_LEQUAL,
    Greater = GL_GREATER,
    GreaterOrEqual = GL_GEQUAL,
    Equal = GL_EQUAL,
    NotEqual = GL_NOTEQUAL,
    Always = GL_ALWAYS
};

// [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glStencilOp.xhtml)
enum class StencilOp : GLenum {
    Keep = GL_KEEP,
    Zero = GL_ZERO,
    Replace = GL_REPLACE,
    Increment = GL_INCR,
    IncrementWrap = GL_INCR_WRAP,
    Decrement = GL_DECR,
    DecrementWrap = GL_DECR_WRAP,
    Invert = GL_INVERT
};

// [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glDepthFunc.xhtml)
enum class DepthFunc : GLenum {
    Never = GL_NEVER,
    Less = GL_LESS,
    LessOrEqual = GL_LEQUAL,
    Greater = GL_GREATER,
    GreaterOrEqual = GL_GEQUAL,
    NotEqual = GL_NOTEQUAL,
    Equal = GL_EQUAL,
    Always = GL_ALWAYS
};

// info about certain features that the system supports
struct Features {
    // the maximum level of anisotropic filtering
    // https://www.khronos.org/opengl/wiki/Sampler_Object#Anisotropic_filtering
    float maxTextureMaxAnisotropy;
};

// See https://doc.magnum.graphics/magnum/opengl-workarounds.html as a reference for workarounds
struct Environment {
    // The gpu vendor. One of intel, nvidia, ati or unknown
    std::string vendor;
    // enables a fix for intel's buggy drivers
    bool useIntelTextureBindingFix;
    // enables a fix for intel's buggy drivers
    bool useIntelCubemapDsaFix;
    Features features;
};

#define VENDOR_INTEL "intel"
#define VENDOR_NVIDIA "nvidia"
// amd used to be called ati and still uses "ati" in the vendor string
#define VENDOR_AMD "ati"
#define VENDOR_UNKNOWN "unknown"

Environment createEnvironment();

// The state manager improves performance by reducing redundant state changes.
// Calling any opengl function has an overhead, especially the glBind* and glUseProgram* functions.
// Additionally, when using setEnabled, it is explicit which capabilities should be enabled and need to be configured.
// This avoids the easy mistake of forgetting to disable a capability in one part of the code.
class StateManager {
#ifndef NDEBUG
    // Tracks created / deleted gl objects
    class Tracker {
        std::map<GLenum, std::set<GLuint>> objects_ = {{GL_BUFFER, {}}, {GL_SHADER, {}}, {GL_PROGRAM, {}}, {GL_VERTEX_ARRAY, {}}, {GL_PROGRAM_PIPELINE, {}}, {GL_SAMPLER, {}}, {GL_TEXTURE, {}}, {GL_RENDERBUFFER, {}}, {GL_FRAMEBUFFER, {}}};

       public:
        void add(GLenum type, GLuint id);

        void remove(GLenum type, GLuint id);

        // returns all the tracked objects
        std::vector<std::pair<GLenum, GLuint>> tracked();
    };

    Tracker tracker_;
#endif  // NDEBUG

   public:
    StateManager(Environment env);
    ~StateManager(){};

    // Keep track of a gl object
    void track(GLenum type, GLuint id) {
#ifndef NDEBUG
        tracker_.add(type, id);
#endif
    }

    // inverse of `track(type, id)`
    void untrack(GLenum type, GLuint id) {
#ifndef NDEBUG
        tracker_.remove(type, id);
#endif
    }

    std::vector<std::pair<GLenum, GLuint>> tracked() {
#ifndef NDEBUG
        return tracker_.tracked();
#else
        return {};
#endif
    }

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glEnable.xhtml)
    void enable(Capability cap);
    // Sets the provided capabilities to enabled, all others will be disabled
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glEnable.xhtml)
    void setEnabled(const std::vector<Capability>& caps);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glEnable.xhtml)
    void disable(Capability cap);
    /**
     * [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glCullFace.xhtml)
     * @param face `GL_FRONT` or `GL_BACK`
     */
    void cull(GLenum face);
    // Cull the front face.
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glCullFace.xhtml)
    void cullFront();
    // Cull the back face.
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glCullFace.xhtml)
    void cullBack();
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBlendFunc.xhtml)
    void blendFunc(BlendFactor sfactor, BlendFactor dfactor);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBlendFuncSeparate.xhtml)
    void blendFuncSeparate(BlendFactor srcRGB, BlendFactor dstRGB, BlendFactor srcAlpha, BlendFactor dstAlpha);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBlendEquation.xhtml)
    void blendEquation(BlendEquation mode);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBlendEquationSeparate.xhtml)
    void blendEquationSeparate(BlendEquation modeRGB, BlendEquation modeAlpha);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glStencilFunc.xhtml)
    void stencilFunc(StencilFunc fn, int32_t ref, uint32_t mask);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glStencilFuncSeparate.xhtml)
    void stencilFuncFront(StencilFunc fn, int32_t ref, uint32_t mask);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glStencilFuncSeparate.xhtml)
    void stencilFuncBack(StencilFunc fn, int32_t ref, uint32_t mask);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glStencilOp.xhtml)
    void stencilOp(StencilOp sfail, StencilOp dpfail, StencilOp dppass);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glStencilOpSeparate.xhtml)
    void stencilOpFront(StencilOp sfail, StencilOp dpfail, StencilOp dppass);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glStencilOpSeparate.xhtml)
    void stencilOpBack(StencilOp sfail, StencilOp dpfail, StencilOp dppass);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glStencilMask.xhtml)
    void stencilMask(uint32_t mask);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glStencilMaskSeparate.xhtml)
    void stencilMaskFront(uint32_t mask);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glStencilMaskSeparate.xhtml)
    void stencilMaskBack(uint32_t mask);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glDepthFunc.xhtml)
    void depthFunc(DepthFunc fn);
    /**
     * [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glDepthMask.xhtml)
     * @param flag `true` enables writing to the depth buffer
     */
    void depthMask(bool flag);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glPolygonMode.xhtml)
    void polygonMode(GLenum face, GLenum mode);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glPolygonOffset.xhtml)
    void polygonOffset(float factor, float units);
    // [Reference](https://registry.khronos.org/OpenGL/extensions/EXT/EXT_polygon_offset_clamp.txt)
    void polygonOffsetClamp(float factor, float units, float clamp);

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glActiveTexture.xhtml)
    void activeTexture(int unit);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBindTextureUnit.xhtml)
    void bindTextureUnit(int unit, GLuint texture);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBindTexture.xhtml)
    void bindTexture(GLenum target, GLuint texture);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBindSampler.xhtml)
    void bindSampler(int unit, GLuint sampler);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBindBuffer.xhtml)
    void bindBuffer(GLenum target, GLuint buffer);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBindBuffer.xhtml)
    void bindArrayBuffer(GLuint buffer);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBindBuffer.xhtml)
    void bindElementArrayBuffer(GLuint buffer);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBindBuffer.xhtml)
    void bindDrawIndirectBuffer(GLuint buffer);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBindBuffer.xhtml)
    void bindCopyReadBuffer(GLuint buffer);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBindBuffer.xhtml)
    void bindCopyWriteBuffer(GLuint buffer);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBindBuffer.xhtml)
    void bindShaderStorageBuffer(GLuint buffer);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBindBuffer.xhtml)
    void bindUniformBuffer(GLuint buffer);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBindBuffer.xhtml)
    void bindTextureBuffer(GLuint buffer);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBindBuffer.xhtml)
    void bindTransformFeedbackBuffer(GLuint buffer);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBindFramebuffer.xhtml)
    void bindFramebuffer(GLenum target, GLuint framebuffer);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBindFramebuffer.xhtml)
    void bindDrawFramebuffer(GLuint framebuffer);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBindFramebuffer.xhtml)
    void bindReadFramebuffer(GLuint framebuffer);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBindRenderbuffer.xhtml)
    void bindRenderbuffer(GLuint renderbuffer);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBindProgramPipeline.xhtml)
    void bindProgramPipeline(GLuint pipeline);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glBindVertexArray.xhtml)
    void bindVertexArray(GLuint array);

    void unbindTexture(GLuint texture);
    void unbindSampler(GLuint sampler);
    void unbindBuffer(GLuint buffer);
    void unbindFramebuffer(GLuint framebuffer);
    void unbindRenderbuffer(GLuint renderbuffer);
    void unbindProgramPipeline(GLuint pipeline);
    void unbindVertexArray(GLuint array);

    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glViewport.xhtml)
    void setViewport(int x, int y, int width, int height);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glScissor.xhtml)
    void setScissor(int x, int y, int width, int height);
    // [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glClearColor.xhtml)
    void setClearColor(float r, float g, float b, float a);

   private:
    Environment env = {};

    std::map<Capability, bool> caps;

    std::vector<GLuint> textureUnits;
    std::vector<GLuint> samplerUnits;
    std::unordered_map<GLuint, GLenum> intelTextureBindingTargets;

    GLuint drawFramebuffer = 0;
    GLuint readFramebuffer = 0;
    GLuint renderbuffer = 0;
    GLuint arrayBuffer = 0;
    GLuint drawIndirectBuffer = 0;
    GLuint elementArrayBuffer = 0;
    GLuint textureBuffer = 0;
    GLuint uniformBuffer = 0;
    GLuint transformFeedbackBuffer = 0;
    GLuint copyReadBuffer = 0;
    GLuint copyWriteBuffer = 0;
    GLuint shaderStorageBuffer = 0;
    GLuint programPipeline = 0;
    GLuint vertexArray = 0;

    int activeTextureUnit = 0;

    std::array<int, 4> viewportRect = {};
    std::array<int, 4> scissorRect = {};

    BlendFactor blendRgbFactorSrc;
    BlendFactor blendAlphaFactorSrc;
    BlendFactor blendRgbFactorDst;
    BlendFactor blendAlphaFactorDst;
    BlendEquation blendEquationRgb;
    BlendEquation blendEquationAlpha;

    StencilFunc stencilFrontFuncFn;
    StencilFunc stencilBackFuncFn;
    uint32_t stencilFrontFuncMask;
    uint32_t stencilBackFuncMask;
    int32_t stencilFrontFuncRef;
    int32_t stencilBackFuncRef;

    StencilOp stencilFrontOpSfail;
    StencilOp stencilFrontOpDpfail;
    StencilOp stencilFrontOpDppass;
    StencilOp stencilBackOpSfail;
    StencilOp stencilBackOpDpfail;
    StencilOp stencilBackOpDppass;

    uint32_t stencilFrontMask;
    uint32_t stencilBackMask;

    DepthFunc depthFuncFn;
    bool depthWriteMask;
    uint32_t cullFaceMask;

    std::array<float, 4> clearColorRgba;

    std::array<float, 3> polygonOffsets;

    GLenum polygonModeFront;
    GLenum polygonModeBack;

   public:
    const Environment& environment() const {
        return env;
    }

    void intelTextureBindingSetTarget(GLuint texture, GLenum target) {
        intelTextureBindingTargets[texture] = target;
    }

    void intelCubemapDsaRebindFramebuffer() {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, readFramebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFramebuffer);
    }

    float clampAnisotropy(float desired) {
        float max = env.features.maxTextureMaxAnisotropy;
        if (desired > max) return max;
        return desired;
    }
};

inline std::unique_ptr<StateManager> manager;

// [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glPushDebugGroup.xhtml)
inline void pushDebugGroup(const std::string& name) {
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, name.c_str());
}

// [Reference](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glPopDebugGroup.xhtml)
inline void popDebugGroup() {
    glPopDebugGroup();
}

}  // namespace gl