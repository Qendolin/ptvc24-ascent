#define GLEW_STATIC
#include <GL/glew.h>

#include <array>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#pragma once

namespace GL {

enum class Capability : GLenum {
    DepthTest = GL_DEPTH_TEST,
    Blend = GL_BLEND,
    StencilTest = GL_STENCIL_TEST,
    ScissorTest = GL_SCISSOR_TEST,
    CullFace = GL_CULL_FACE,
    DepthClamp = GL_DEPTH_CLAMP,
    PolygonOffsetFill = GL_POLYGON_OFFSET_FILL
};

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

enum class BlendEquation : GLenum {
    FuncAdd = GL_FUNC_ADD,
    FuncSubtract = GL_FUNC_SUBTRACT,
    FuncReverseSubtract = GL_FUNC_REVERSE_SUBTRACT,
    Min = GL_MIN,
    Max = GL_MAX
};

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

struct Features {
    float maxTextureMaxAnisotropy;
};

// See https://doc.magnum.graphics/magnum/opengl-workarounds.html as a reference for workarounds
struct Environment {
    std::string vendor;
    bool useIntelTextureBindingFix;
    bool useIntelCubemapDsaFix;
    Features features;
};

#define VENDOR_INTEL "intel"
#define VENDOR_NVIDIA "nvidia"
#define VENDOR_AMD "ati"
#define VENDOR_UNKNOWN "unknown"

Environment createEnvironment();

class StateManager {
   public:
    StateManager(Environment env);
    ~StateManager();

    void enable(Capability cap);
    void setEnabled(const std::vector<Capability>& caps);
    void disable(Capability cap);
    void cullFront();
    void cullBack();
    void blendFunc(BlendFactor sfactor, BlendFactor dfactor);
    void blendFuncSeparate(BlendFactor srcRGB, BlendFactor dstRGB, BlendFactor srcAlpha, BlendFactor dstAlpha);
    void blendEquation(BlendEquation mode);
    void blendEquationSeparate(BlendEquation modeRGB, BlendEquation modeAlpha);
    void stencilFunc(StencilFunc fn, int32_t ref, uint32_t mask);
    void stencilFuncFront(StencilFunc fn, int32_t ref, uint32_t mask);
    void stencilFuncBack(StencilFunc fn, int32_t ref, uint32_t mask);
    void stencilOp(StencilOp sfail, StencilOp dpfail, StencilOp dppass);
    void stencilOpFront(StencilOp sfail, StencilOp dpfail, StencilOp dppass);
    void stencilOpBack(StencilOp sfail, StencilOp dpfail, StencilOp dppass);
    void stencilMask(uint32_t mask);
    void stencilMaskFront(uint32_t mask);
    void stencilMaskBack(uint32_t mask);
    void depthFunc(DepthFunc fn);
    void depthMask(bool flag);
    void polygonMode(GLenum face, GLenum mode);
    void polygonOffset(float factor, float units);
    void polygonOffsetClamp(float factor, float units, float clamp);

    void activeTexture(int unit);
    void bindTextureUnit(int unit, GLuint texture);
    void bindTexture(GLenum target, GLuint texture);
    void bindSampler(int unit, GLuint sampler);
    void bindBuffer(GLenum target, GLuint buffer);
    void bindArrayBuffer(GLuint buffer);
    void bindElementArrayBuffer(GLuint buffer);
    void bindDrawIndirectBuffer(GLuint buffer);
    void bindCopyReadBuffer(GLuint buffer);
    void bindCopyWriteBuffer(GLuint buffer);
    void bindShaderStorageBuffer(GLuint buffer);
    void bindUniformBuffer(GLuint buffer);
    void bindTextureBuffer(GLuint buffer);
    void bindTransformFeedbackBuffer(GLuint buffer);
    void bindFramebuffer(GLenum target, GLuint framebuffer);
    void bindDrawFramebuffer(GLuint framebuffer);
    void bindReadFramebuffer(GLuint framebuffer);
    void bindRenderbuffer(GLuint renderbuffer);
    void bindProgramPipeline(GLuint pipeline);
    void bindVertexArray(GLuint array);

    void unbindTexture(GLuint texture);
    void unbindSampler(GLuint sampler);
    void unbindBuffer(GLuint buffer);
    void unbindFramebuffer(GLuint framebuffer);
    void unbindRenderbuffer(GLuint renderbuffer);
    void unbindProgramPipeline(GLuint pipeline);
    void unbindVertexArray(GLuint array);

    void setViewport(int x, int y, int width, int height);
    void setScissor(int x, int y, int width, int height);
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
};

inline std::unique_ptr<StateManager> manager;

inline void pushDebugGroup(const std::string& name) {
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, name.c_str());
}

inline void popDebugGroup() {
    glPopDebugGroup();
}

}  // namespace GL