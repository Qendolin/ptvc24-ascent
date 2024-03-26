#include "StateManager.h"

#include <algorithm>

namespace GL {

Environment createEnvironment() {
    std::string vendor = std::string(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    vendor = std::string(vendor.begin(), std::find(vendor.begin(), vendor.end(), '\0'));  // Remove trailing null character
    std::transform(vendor.begin(), vendor.end(), vendor.begin(), ::tolower);

    if (vendor.find("intel") != std::string::npos) {
        vendor = VENDOR_INTEL;
    } else if (vendor.find("nvidia ") != std::string::npos) {
        vendor = VENDOR_NVIDIA;
    } else if (vendor.find("ati ") != std::string::npos) {
        vendor = VENDOR_AMD;
    } else {
        vendor = VENDOR_UNKNOWN;
    }

    Features features;
    std::vector<float> anisotropy(1);  // Assuming gl.h doesn't define GetFloatv
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, anisotropy.data());
    features.maxTextureMaxAnisotropy = anisotropy[0];

    return Environment{
        .vendor = vendor,
        .useIntelTextureBindingFix = vendor == VENDOR_INTEL,
        .useIntelCubemapDsaFix = vendor == VENDOR_INTEL,
        .features = features};
}

StateManager::StateManager(Environment env) : caps(),
                                              textureUnits(32, 0),
                                              samplerUnits(32, 0),
                                              intelTextureBindingTargets() {
    this->env = env;
}

StateManager::~StateManager() {
}

void StateManager::enable(Capability cap) {
    if (caps.count(cap) && caps.at(cap)) {
        return;
    }

    glEnable(static_cast<GLenum>(cap));
    caps[cap] = true;
}

void StateManager::setEnabled(const std::vector<Capability>& caps) {
    std::unordered_map<Capability, bool> diff;

    // Assume all disabled
    for (const auto& [capability, enabled] : this->caps) {
        if (enabled) {
            diff[capability] = false;
        }
    }

    // enable provided
    for (Capability cap : caps) {
        if (!diff[cap] || !this->caps[cap]) {
            diff[cap] = true;
        }
    }

    // Enable or disable capabilities based on the diff map
    for (const auto& [capability, enable] : diff) {
        if (enable) {
            this->enable(capability);
        } else {
            this->disable(capability);
        }
    }
}

void StateManager::disable(Capability cap) {
    if (caps.count(cap) && !caps.at(cap)) {
        return;
    }

    glDisable(static_cast<GLenum>(cap));
    caps[cap] = false;
}

void StateManager::cull(GLenum face) {
    if (cullFaceMask == face) {
        return;
    }
    glCullFace(face);
    cullFaceMask = face;
}

void StateManager::cullFront() {
    if (cullFaceMask == GL_FRONT) {
        return;
    }
    glCullFace(GL_FRONT);
    cullFaceMask = GL_FRONT;
}

void StateManager::cullBack() {
    if (cullFaceMask == GL_BACK) {
        return;
    }
    glCullFace(GL_BACK);
    cullFaceMask = GL_BACK;
}

void StateManager::blendFunc(BlendFactor sfactor, BlendFactor dfactor) {
    if (blendAlphaFactorSrc == sfactor && blendRgbFactorSrc == sfactor && blendAlphaFactorDst == dfactor && blendRgbFactorDst == dfactor) {
        return;
    }
    glBlendFunc(static_cast<GLenum>(sfactor), static_cast<GLenum>(dfactor));
    blendAlphaFactorSrc = blendRgbFactorSrc = sfactor;
    blendAlphaFactorDst = blendRgbFactorDst = dfactor;
}

void StateManager::blendFuncSeparate(BlendFactor srcRgb, BlendFactor dstRgb, BlendFactor srcAlpha, BlendFactor dstAlpha) {
    if (blendAlphaFactorSrc == srcAlpha && blendRgbFactorSrc == srcRgb && blendAlphaFactorDst == dstAlpha && blendRgbFactorDst == dstRgb) {
        return;
    }
    glBlendFuncSeparate(static_cast<GLenum>(srcRgb), static_cast<GLenum>(dstRgb), static_cast<GLenum>(srcAlpha), static_cast<GLenum>(dstAlpha));
    blendAlphaFactorSrc = srcAlpha;
    blendRgbFactorSrc = srcRgb;
    blendAlphaFactorDst = dstAlpha;
    blendRgbFactorDst = dstRgb;
}

void StateManager::blendEquation(BlendEquation mode) {
    if (blendEquationAlpha == mode && blendEquationRgb == mode) {
        return;
    }
    glBlendEquation(static_cast<GLenum>(mode));
    blendEquationAlpha = mode;
    blendEquationRgb = mode;
}

void StateManager::blendEquationSeparate(BlendEquation modeRGB, BlendEquation modeAlpha) {
    if (blendEquationAlpha == modeAlpha && blendEquationRgb == modeRGB) {
        return;
    }
    glBlendEquationSeparate(static_cast<GLenum>(modeRGB), static_cast<GLenum>(modeAlpha));
    blendEquationAlpha = modeAlpha;
    blendEquationRgb = modeRGB;
}

void StateManager::stencilFunc(StencilFunc fn, int32_t ref, uint32_t mask) {
    if (stencilFrontFuncFn == fn && stencilFrontFuncRef == ref && stencilFrontFuncMask == mask &&
        stencilBackFuncFn == fn && stencilBackFuncRef == ref && stencilBackFuncMask == mask) {
        return;
    }

    glStencilFunc(static_cast<GLenum>(fn), ref, mask);

    stencilFrontFuncFn = fn;
    stencilFrontFuncRef = ref;
    stencilFrontFuncMask = mask;
    stencilBackFuncFn = fn;
    stencilBackFuncRef = ref;
    stencilBackFuncMask = mask;
}

void StateManager::stencilFuncFront(StencilFunc fn, int32_t ref, uint32_t mask) {
    if (stencilFrontFuncFn == fn && stencilFrontFuncRef == ref && stencilFrontFuncMask == mask) {
        return;
    }

    glStencilFuncSeparate(GL_FRONT, static_cast<GLenum>(fn), ref, mask);

    stencilFrontFuncFn = fn;
    stencilFrontFuncRef = ref;
    stencilFrontFuncMask = mask;
}

void StateManager::stencilFuncBack(StencilFunc fn, int32_t ref, uint32_t mask) {
    if (stencilBackFuncFn == fn && stencilBackFuncRef == ref && stencilBackFuncMask == mask) {
        return;
    }

    glStencilFuncSeparate(GL_BACK, static_cast<GLenum>(fn), ref, mask);

    stencilBackFuncFn = fn;
    stencilBackFuncRef = ref;
    stencilBackFuncMask = mask;
}

void StateManager::stencilOp(StencilOp sfail, StencilOp dpfail, StencilOp dppass) {
    if (sfail == stencilFrontOpSfail && dpfail == stencilFrontOpDpfail && dppass == stencilFrontOpDppass &&
        sfail == stencilBackOpSfail && dpfail == stencilBackOpDpfail && dppass == stencilBackOpDppass) {
        return;
    }
    glStencilOp(static_cast<GLenum>(sfail), static_cast<GLenum>(dpfail), static_cast<GLenum>(dppass));
    stencilFrontOpSfail = sfail;
    stencilFrontOpDpfail = dpfail;
    stencilFrontOpDppass = dppass;
    stencilBackOpSfail = sfail;
    stencilBackOpDpfail = dpfail;
    stencilBackOpDppass = dppass;
}

void StateManager::stencilOpFront(StencilOp sfail, StencilOp dpfail, StencilOp dppass) {
    if (sfail == stencilFrontOpSfail && dpfail == stencilFrontOpDpfail && dppass == stencilFrontOpDppass) {
        return;
    }
    glStencilOpSeparate(GL_FRONT, static_cast<GLenum>(sfail), static_cast<GLenum>(dpfail), static_cast<GLenum>(dppass));
    stencilFrontOpSfail = sfail;
    stencilFrontOpDpfail = dpfail;
    stencilFrontOpDppass = dppass;
}

void StateManager::stencilOpBack(StencilOp sfail, StencilOp dpfail, StencilOp dppass) {
    if (sfail == stencilBackOpSfail && dpfail == stencilBackOpDpfail && dppass == stencilBackOpDppass) {
        return;
    }
    glStencilOpSeparate(GL_BACK, static_cast<GLenum>(sfail), static_cast<GLenum>(dpfail), static_cast<GLenum>(dppass));
    stencilBackOpSfail = sfail;
    stencilBackOpDpfail = dpfail;
    stencilBackOpDppass = dppass;
}

void StateManager::stencilMask(uint32_t mask) {
    if (stencilFrontMask == mask && stencilBackMask == mask) {
        return;
    }
    glStencilMask(mask);
    stencilFrontMask = mask;
    stencilBackMask = mask;
}

void StateManager::stencilMaskFront(uint32_t mask) {
    if (stencilFrontMask == mask) {
        return;
    }
    glStencilMaskSeparate(GL_FRONT, mask);
    stencilFrontMask = mask;
}

void StateManager::stencilMaskBack(uint32_t mask) {
    if (stencilBackMask == mask) {
        return;
    }
    glStencilMaskSeparate(GL_BACK, mask);
    stencilBackMask = mask;
}

void StateManager::depthFunc(DepthFunc fn) {
    if (depthFuncFn == fn) {
        return;
    }
    glDepthFunc(static_cast<GLenum>(fn));
    depthFuncFn = fn;
}

void StateManager::depthMask(bool flag) {
    if (depthWriteMask == flag) {
        return;
    }
    glDepthMask(flag);
    depthWriteMask = flag;
}

void StateManager::polygonMode(GLenum face, GLenum mode) {
    if (face == GL_FRONT_AND_BACK &&
        (polygonModeFront != mode || polygonModeBack != mode)) {
        glPolygonMode(face, mode);
        polygonModeBack = mode;
        polygonModeFront = mode;
    } else if (face == GL_FRONT && polygonModeFront != mode) {
        glPolygonMode(face, mode);
        polygonModeFront = mode;
    } else if (face == GL_BACK && polygonModeBack != mode) {
        glPolygonMode(face, mode);
        polygonModeBack = mode;
    }
}

void StateManager::polygonOffset(float factor, float units) {
    if (polygonOffsets[0] == factor && polygonOffsets[1] == units &&
        polygonOffsets[2] == 0) {
        return;
    }
    glPolygonOffset(factor, units);
    polygonOffsets = {factor, units, 0};
}

void StateManager::polygonOffsetClamp(float factor, float units, float clamp) {
    if (polygonOffsets[0] == factor && polygonOffsets[1] == units &&
        polygonOffsets[2] == clamp) {
        return;
    }
    glPolygonOffsetClamp(factor, units, clamp);
    polygonOffsets = {factor, units, clamp};
}

void StateManager::bindTextureUnit(int unit, GLuint texture) {
    if (textureUnits[unit] == texture) {
        return;
    }

    if (env.useIntelTextureBindingFix) {
        activeTexture(unit);
        if (texture == 0) {
            // Not sure if this is without issues
            textureUnits[unit] = texture;
            return;
        }
        glBindTexture(intelTextureBindingTargets[texture], texture);
        textureUnits[unit] = texture;
        return;
    }

    glBindTextureUnit(unit, texture);
    textureUnits[unit] = texture;
}

void StateManager::bindTexture(GLenum target, GLuint texture) {
    if (textureUnits[activeTextureUnit] == texture) {
        return;
    }
    glBindTexture(target, texture);
    textureUnits[activeTextureUnit] = texture;
}

void StateManager::activeTexture(int unit) {
    if (activeTextureUnit == unit) {
        return;
    }
    glActiveTexture(GL_TEXTURE0 + unit);
    activeTextureUnit = unit;
}

void StateManager::bindSampler(int unit, GLuint sampler) {
    if (samplerUnits[unit] == sampler) {
        return;
    }
    glBindSampler(static_cast<uint32_t>(unit), sampler);
    samplerUnits[unit] = sampler;
}

void StateManager::bindBuffer(GLenum target, GLuint buffer) {
    switch (target) {
        case GL_ARRAY_BUFFER:
            bindArrayBuffer(buffer);
            break;
        case GL_ELEMENT_ARRAY_BUFFER:
            bindElementArrayBuffer(buffer);
            break;
        case GL_DRAW_INDIRECT_BUFFER:
            bindDrawIndirectBuffer(buffer);
            break;
        case GL_COPY_READ_BUFFER:
            bindCopyReadBuffer(buffer);
            break;
        case GL_COPY_WRITE_BUFFER:
            bindCopyWriteBuffer(buffer);
            break;
        case GL_UNIFORM_BUFFER:
            bindUniformBuffer(buffer);
            break;
        case GL_SHADER_STORAGE_BUFFER:
            bindShaderStorageBuffer(buffer);
            break;
        case GL_TEXTURE_BUFFER:
            bindTextureBuffer(buffer);
            break;
        case GL_TRANSFORM_FEEDBACK_BUFFER:
            bindTransformFeedbackBuffer(buffer);
            break;
        default:
            glBindBuffer(target, buffer);
    }
}

void StateManager::bindArrayBuffer(GLuint buffer) {
    if (arrayBuffer == buffer) {
        return;
    }
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    arrayBuffer = buffer;
}

void StateManager::bindElementArrayBuffer(GLuint buffer) {
    if (elementArrayBuffer == buffer) {
        return;
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
    elementArrayBuffer = buffer;
}

void StateManager::bindDrawIndirectBuffer(GLuint buffer) {
    if (drawIndirectBuffer == buffer) {
        return;
    }
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buffer);
    drawIndirectBuffer = buffer;
}

void StateManager::bindCopyReadBuffer(GLuint buffer) {
    if (copyReadBuffer == buffer) {
        return;
    }
    glBindBuffer(GL_COPY_READ_BUFFER, buffer);
    copyReadBuffer = buffer;
}

void StateManager::bindCopyWriteBuffer(GLuint buffer) {
    if (copyWriteBuffer == buffer) {
        return;
    }
    glBindBuffer(GL_COPY_WRITE_BUFFER, buffer);
    copyWriteBuffer = buffer;
}

void StateManager::bindShaderStorageBuffer(GLuint buffer) {
    if (shaderStorageBuffer == buffer) {
        return;
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
    shaderStorageBuffer = buffer;
}

void StateManager::bindUniformBuffer(GLuint buffer) {
    if (uniformBuffer == buffer) {
        return;
    }
    glBindBuffer(GL_UNIFORM_BUFFER, buffer);
    uniformBuffer = buffer;
}

void StateManager::bindTextureBuffer(GLuint buffer) {
    if (textureBuffer == buffer) {
        return;
    }
    glBindBuffer(GL_TEXTURE_BUFFER, buffer);
    textureBuffer = buffer;
}

void StateManager::bindTransformFeedbackBuffer(GLuint buffer) {
    if (transformFeedbackBuffer == buffer) {
        return;
    }
    glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, buffer);
    transformFeedbackBuffer = buffer;
}

void StateManager::bindFramebuffer(GLenum target, GLuint framebuffer) {
    if (target == GL_DRAW_FRAMEBUFFER) {
        bindDrawFramebuffer(framebuffer);
    } else if (target == GL_READ_FRAMEBUFFER) {
        bindReadFramebuffer(framebuffer);
    } else {
        if (framebuffer == drawFramebuffer && framebuffer == readFramebuffer) {
            return;
        }
        glBindFramebuffer(target, framebuffer);
        drawFramebuffer = framebuffer;
        readFramebuffer = framebuffer;
    }
}

void StateManager::bindDrawFramebuffer(GLuint framebuffer) {
    if (drawFramebuffer == framebuffer) {
        return;
    }
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
    drawFramebuffer = framebuffer;
}

void StateManager::bindReadFramebuffer(GLuint framebuffer) {
    if (readFramebuffer == framebuffer) {
        return;
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
    readFramebuffer = framebuffer;
}

void StateManager::bindRenderbuffer(GLuint renderbuffer) {
    if (this->renderbuffer == renderbuffer) {
        return;
    }
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
    this->renderbuffer = renderbuffer;
}

void StateManager::bindProgramPipeline(GLuint pipeline) {
    if (programPipeline == pipeline) {
        return;
    }
    glBindProgramPipeline(pipeline);
    programPipeline = pipeline;
}

void StateManager::bindVertexArray(GLuint array) {
    if (vertexArray == array) {
        return;
    }
    glBindVertexArray(array);
    vertexArray = array;
}

void StateManager::unbindTexture(GLuint texture) {
    for (size_t unit = 0; unit < textureUnits.size(); unit++) {
        if (textureUnits[unit] == texture) {
            textureUnits[unit] = 0;
            glBindTextureUnit(unit, 0);
        }
    }

    if (env.useIntelTextureBindingFix && intelTextureBindingTargets.count(texture)) {
        intelTextureBindingTargets.erase(texture);
    }
}

void StateManager::unbindSampler(GLuint sampler) {
    for (size_t unit = 0; unit < samplerUnits.size(); unit++) {
        if (samplerUnits[unit] == sampler) {
            samplerUnits[unit] = 0;
            glBindSampler(unit, 0);
        }
    }
}

void StateManager::unbindBuffer(GLuint buffer) {
    if (arrayBuffer == buffer) {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        arrayBuffer = 0;
    }
    if (elementArrayBuffer == buffer) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        elementArrayBuffer = 0;
    }
    if (drawIndirectBuffer == buffer) {
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
        drawIndirectBuffer = 0;
    }
    if (copyReadBuffer == buffer) {
        glBindBuffer(GL_COPY_READ_BUFFER, 0);
        copyReadBuffer = 0;
    }
    if (copyWriteBuffer == buffer) {
        glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
        copyWriteBuffer = 0;
    }
    if (uniformBuffer == buffer) {
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        uniformBuffer = 0;
    }
    if (shaderStorageBuffer == buffer) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        shaderStorageBuffer = 0;
    }
    if (textureBuffer == buffer) {
        glBindBuffer(GL_TEXTURE_BUFFER, 0);
        textureBuffer = 0;
    }
    if (transformFeedbackBuffer == buffer) {
        glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);
        transformFeedbackBuffer = 0;
    }
}

void StateManager::unbindFramebuffer(GLuint framebuffer) {
    if (drawFramebuffer == framebuffer) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
        drawFramebuffer = 0;
    }
    if (readFramebuffer == framebuffer) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
        readFramebuffer = 0;
    }
}

void StateManager::unbindRenderbuffer(GLuint renderbuffer) {
    if (this->renderbuffer == renderbuffer) {
        this->renderbuffer = 0;
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }
}

void StateManager::unbindProgramPipeline(GLuint pipeline) {
    if (this->programPipeline == pipeline) {
        this->programPipeline = 0;
        glBindProgramPipeline(0);
    }
}

void StateManager::unbindVertexArray(GLuint array) {
    if (this->vertexArray == array) {
        this->vertexArray = 0;
        glBindVertexArray(0);
    }
}

void StateManager::setViewport(int x, int y, int width, int height) {
    if (viewportRect[0] == x && viewportRect[1] == y &&
        viewportRect[2] == width && viewportRect[3] == height) {
        return;
    }

    glViewport(static_cast<GLint>(x), static_cast<GLint>(y),
               static_cast<GLsizei>(width), static_cast<GLsizei>(height));
    viewportRect = {x, y, width, height};
}

void StateManager::setScissor(int x, int y, int width, int height) {
    if (scissorRect[0] == x && scissorRect[1] == y &&
        scissorRect[2] == width && scissorRect[3] == height) {
        return;
    }

    glScissor(static_cast<GLint>(x), static_cast<GLint>(y),
              static_cast<GLsizei>(width), static_cast<GLsizei>(height));
    scissorRect = {x, y, width, height};
}

void StateManager::setClearColor(float r, float g, float b, float a) {
    if (clearColorRgba[0] == r && clearColorRgba[1] == g &&
        clearColorRgba[2] == b && clearColorRgba[3] == a) {
        return;
    }

    glClearColor(r, g, b, a);
    clearColorRgba = {r, g, b, a};
}

}  // namespace GL
