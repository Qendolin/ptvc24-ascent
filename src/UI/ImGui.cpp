#include "ImGUI.h"

#include <imgui.h>

#include "../GL/Geometry.h"
#include "../GL/Shader.h"
#include "../GL/StateManager.h"
#include "../GL/Texture.h"
#include "../Input.h"
#include "../Utils.h"
#include "../Window.h"

static ImGuiKey mapGlfwKey(int key);

namespace ui {

ImGuiBackend::ImGuiBackend() {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
#ifndef NDEBUG
    io.IniFilename = INI_FILENAME;
    io.LogFilename = LOG_FILENAME;
#else
    // disable files in release mode
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
#endif
    io.DisplaySize.x = 1600;
    io.DisplaySize.y = 900;
    ImGui::StyleColorsDark();

    shader_ = new gl::ShaderPipeline({
        new gl::ShaderProgram("assets/shaders/imgui.vert"),
        new gl::ShaderProgram("assets/shaders/imgui.frag"),
    });
    shader_->setDebugLabel("imgui/shader");

    vao_ = new gl::VertexArray();
    vao_->setDebugLabel("imgui/vao");
    vao_->layout(0, 0, 2, GL_FLOAT, false, offsetof(ImDrawVert, pos));
    vao_->layout(0, 1, 2, GL_FLOAT, false, offsetof(ImDrawVert, uv));
    vao_->layout(0, 2, 4, GL_UNSIGNED_BYTE, true, offsetof(ImDrawVert, col));

    vbo_ = new gl::Buffer();
    vbo_->setDebugLabel("imgui/vbo");
    vbo_->allocateEmpty(1024 * 8 * sizeof(ImDrawVert), GL_DYNAMIC_STORAGE_BIT);
    vao_->bindBuffer(0, *vbo_, 0, sizeof(ImDrawVert));

    ebo_ = new gl::Buffer();
    ebo_->setDebugLabel("imgui/ebo");
    ebo_->allocateEmpty(1024 * 8 * sizeof(uint32_t), GL_DYNAMIC_STORAGE_BIT);
    vao_->bindElementBuffer(*ebo_);

    struct {
        uint8_t* data;
        int width;
        int height;
    } font_atlas_image;
    io.Fonts->GetTexDataAsRGBA32(&font_atlas_image.data, &font_atlas_image.width, &font_atlas_image.height);

    fontAtlas_ = new gl::Texture(GL_TEXTURE_2D);
    fontAtlas_->setDebugLabel("imgui/font_atlas");
    fontAtlas_->allocate(1, GL_RGBA8, font_atlas_image.width, font_atlas_image.height);
    fontAtlas_->load(0, font_atlas_image.width, font_atlas_image.height, GL_RGBA, GL_UNSIGNED_BYTE, font_atlas_image.data);
    io.Fonts->SetTexID((ImTextureID)(intptr_t)fontAtlas_->id());
}

ImGuiBackend::~ImGuiBackend() {
    if (hasCallbacksBound_) {
        LOG("Forgot to call unbind, callbacks are stil registered!");
    }

    ImGui::DestroyContext();

    delete fontAtlas_;
    delete vao_;
    delete vbo_;
    delete ebo_;
    delete shader_;
}

void ImGuiBackend::setViewport(int width, int height) {
    this->viewport_ = {width, height};
    float w = static_cast<float>(width);
    float h = static_cast<float>(height);
    // clang-format off
    projectionMatrix_ = glm::mat4(
        2 / w,      0,    0,    0,
            0, -2 / h,    0,    0,
            0,      0,    1,    0,
           -1,      1,    0,    1);
    // clang-format on
}

void ImGuiBackend::update(Input& input) {
    ImGui::NewFrame();
    if (input.isMouseCaptured()) {
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
    } else {
        ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
    }
}

bool ImGuiBackend::shouldShowCursor() {
    return ImGui::GetIO().WantCaptureMouse;
}

void ImGuiBackend::render() {
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize.x = static_cast<float>(viewport_.x);
    io.DisplaySize.y = static_cast<float>(viewport_.y);

    double time = glfwGetTime();
    if (lastFrameTime_ != 0.0)
        io.DeltaTime = static_cast<float>(time - lastFrameTime_);
    lastFrameTime_ = time;

    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();

    gl::pushDebugGroup("ImGui::Draw");
    gl::manager->setEnabled({gl::Capability::Blend, gl::Capability::ScissorTest});
    gl::manager->blendEquation(gl::BlendEquation::FuncAdd);
    gl::manager->blendFuncSeparate(gl::BlendFactor::SrcAlpha, gl::BlendFactor::OneMinusSrcAlpha, gl::BlendFactor::One, gl::BlendFactor::OneMinusSrcAlpha);

    vao_->bind();
    shader_->bind();
    shader_->vertexStage()->setUniform("u_projection_mat", projectionMatrix_);

    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0)
        return;

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos;          // (0,0) unless using multi-viewports
    ImVec2 clip_scale = draw_data->FramebufferScale;  // (1,1) unless using retina display which are often (2,2)

    GLenum index_type = sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;

    for (auto&& list : draw_data->CmdLists) {
        // update vbo
        size_t vertex_data_size = list->VtxBuffer.size_in_bytes();
        if (vbo_->grow(vertex_data_size)) {
            vao_->reBindBuffer(0, *vbo_);
        }
        vbo_->write(0, list->VtxBuffer.Data, vertex_data_size);

        // update ebo
        size_t index_data_size = list->IdxBuffer.size_in_bytes();
        if (ebo_->grow(index_data_size)) {
            vao_->bindElementBuffer(*ebo_);
        }
        ebo_->write(0, list->IdxBuffer.Data, index_data_size);

        for (auto&& cmd : list->CmdBuffer) {
            if (cmd.UserCallback != nullptr) {
                if (cmd.UserCallback == ImDrawCallback_ResetRenderState) {
                    gl::manager->setScissor(0, 0, fb_width, fb_height);
                } else {
                    cmd.UserCallback(list, &cmd);
                }
            } else {
                gl::manager->bindTextureUnit(0, (unsigned int)(intptr_t)cmd.GetTexID());

                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min((cmd.ClipRect.x - clip_off.x) * clip_scale.x, (cmd.ClipRect.y - clip_off.y) * clip_scale.y);
                ImVec2 clip_max((cmd.ClipRect.z - clip_off.x) * clip_scale.x, (cmd.ClipRect.w - clip_off.y) * clip_scale.y);
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                    continue;

                // Apply scissor/clipping rectangle (Y is inverted in OpenGL)
                gl::manager->setScissor((int)clip_min.x, (int)((float)fb_height - clip_max.y), (int)(clip_max.x - clip_min.x), (int)(clip_max.y - clip_min.y));
                glDrawElementsBaseVertex(GL_TRIANGLES, cmd.ElemCount, index_type, (void*)(cmd.IdxOffset * sizeof(ImDrawIdx)), cmd.VtxOffset);
            }
        }
    }

    gl::popDebugGroup();
}

void ImGuiBackend::bind(Input& input) {
    if (hasCallbacksBound_) {
        PANIC("Already bound");
    }
    hasCallbacksBound_ = true;
    mouseButtonCallbackID_ = input.addMouseButtonCallback([](int button, int action, int mods) {
        ImGuiIO& io = ImGui::GetIO();
        io.AddMouseButtonEvent(button, action == GLFW_PRESS);
    });
    mousePosCallbackID_ = input.addMousePosCallback([](float x, float y) {
        ImGuiIO& io = ImGui::GetIO();
        io.AddMousePosEvent(x, y);
    });
    scrollCallbackID_ = input.addScrollCallback([](float x, float y) {
        ImGuiIO& io = ImGui::GetIO();
        io.AddMouseWheelEvent(x, y);
    });
    keyCallbackID_ = input.addKeyCallback([](int key, int scancode, int action, int mods) {
        if (action == GLFW_REPEAT) return;
        ImGuiIO& io = ImGui::GetIO();
        io.AddKeyEvent(mapGlfwKey(key), action == GLFW_PRESS);
        io.AddKeyEvent(ImGuiMod_Ctrl, (mods & GLFW_MOD_CONTROL) != 0);
        io.AddKeyEvent(ImGuiMod_Shift, (mods & GLFW_MOD_SHIFT) != 0);
        io.AddKeyEvent(ImGuiMod_Alt, (mods & GLFW_MOD_ALT) != 0);
        io.AddKeyEvent(ImGuiMod_Super, (mods & GLFW_MOD_SUPER) != 0);
    });
    charCallbackID_ = input.addCharCallback([](unsigned int codepoint) {
        ImGuiIO& io = ImGui::GetIO();
        io.AddInputCharacter(codepoint);
    });
}

void ImGuiBackend::unbind(Input& input) {
    if (!hasCallbacksBound_) {
        PANIC("Already unbound");
    }
    hasCallbacksBound_ = false;
    input.removeCallback(mouseButtonCallbackID_);
    input.removeCallback(mousePosCallbackID_);
    input.removeCallback(scrollCallbackID_);
    input.removeCallback(keyCallbackID_);
    input.removeCallback(charCallbackID_);
}

}  // namespace ui

static ImGuiKey mapGlfwKey(int key) {
    switch (key) {
        case GLFW_KEY_TAB:
            return ImGuiKey_Tab;
        case GLFW_KEY_LEFT:
            return ImGuiKey_LeftArrow;
        case GLFW_KEY_RIGHT:
            return ImGuiKey_RightArrow;
        case GLFW_KEY_UP:
            return ImGuiKey_UpArrow;
        case GLFW_KEY_DOWN:
            return ImGuiKey_DownArrow;
        case GLFW_KEY_PAGE_UP:
            return ImGuiKey_PageUp;
        case GLFW_KEY_PAGE_DOWN:
            return ImGuiKey_PageDown;
        case GLFW_KEY_HOME:
            return ImGuiKey_Home;
        case GLFW_KEY_END:
            return ImGuiKey_End;
        case GLFW_KEY_INSERT:
            return ImGuiKey_Insert;
        case GLFW_KEY_DELETE:
            return ImGuiKey_Delete;
        case GLFW_KEY_BACKSPACE:
            return ImGuiKey_Backspace;
        case GLFW_KEY_SPACE:
            return ImGuiKey_Space;
        case GLFW_KEY_ENTER:
            return ImGuiKey_Enter;
        case GLFW_KEY_ESCAPE:
            return ImGuiKey_Escape;
        case GLFW_KEY_APOSTROPHE:
            return ImGuiKey_Apostrophe;
        case GLFW_KEY_COMMA:
            return ImGuiKey_Comma;
        case GLFW_KEY_MINUS:
            return ImGuiKey_Minus;
        case GLFW_KEY_PERIOD:
            return ImGuiKey_Period;
        case GLFW_KEY_SLASH:
            return ImGuiKey_Slash;
        case GLFW_KEY_SEMICOLON:
            return ImGuiKey_Semicolon;
        case GLFW_KEY_EQUAL:
            return ImGuiKey_Equal;
        case GLFW_KEY_LEFT_BRACKET:
            return ImGuiKey_LeftBracket;
        case GLFW_KEY_BACKSLASH:
            return ImGuiKey_Backslash;
        case GLFW_KEY_RIGHT_BRACKET:
            return ImGuiKey_RightBracket;
        case GLFW_KEY_GRAVE_ACCENT:
            return ImGuiKey_GraveAccent;
        case GLFW_KEY_CAPS_LOCK:
            return ImGuiKey_CapsLock;
        case GLFW_KEY_SCROLL_LOCK:
            return ImGuiKey_ScrollLock;
        case GLFW_KEY_NUM_LOCK:
            return ImGuiKey_NumLock;
        case GLFW_KEY_PRINT_SCREEN:
            return ImGuiKey_PrintScreen;
        case GLFW_KEY_PAUSE:
            return ImGuiKey_Pause;
        case GLFW_KEY_KP_0:
            return ImGuiKey_Keypad0;
        case GLFW_KEY_KP_1:
            return ImGuiKey_Keypad1;
        case GLFW_KEY_KP_2:
            return ImGuiKey_Keypad2;
        case GLFW_KEY_KP_3:
            return ImGuiKey_Keypad3;
        case GLFW_KEY_KP_4:
            return ImGuiKey_Keypad4;
        case GLFW_KEY_KP_5:
            return ImGuiKey_Keypad5;
        case GLFW_KEY_KP_6:
            return ImGuiKey_Keypad6;
        case GLFW_KEY_KP_7:
            return ImGuiKey_Keypad7;
        case GLFW_KEY_KP_8:
            return ImGuiKey_Keypad8;
        case GLFW_KEY_KP_9:
            return ImGuiKey_Keypad9;
        case GLFW_KEY_KP_DECIMAL:
            return ImGuiKey_KeypadDecimal;
        case GLFW_KEY_KP_DIVIDE:
            return ImGuiKey_KeypadDivide;
        case GLFW_KEY_KP_MULTIPLY:
            return ImGuiKey_KeypadMultiply;
        case GLFW_KEY_KP_SUBTRACT:
            return ImGuiKey_KeypadSubtract;
        case GLFW_KEY_KP_ADD:
            return ImGuiKey_KeypadAdd;
        case GLFW_KEY_KP_ENTER:
            return ImGuiKey_KeypadEnter;
        case GLFW_KEY_KP_EQUAL:
            return ImGuiKey_KeypadEqual;
        case GLFW_KEY_LEFT_SHIFT:
            return ImGuiKey_LeftShift;
        case GLFW_KEY_LEFT_CONTROL:
            return ImGuiKey_LeftCtrl;
        case GLFW_KEY_LEFT_ALT:
            return ImGuiKey_LeftAlt;
        case GLFW_KEY_LEFT_SUPER:
            return ImGuiKey_LeftSuper;
        case GLFW_KEY_RIGHT_SHIFT:
            return ImGuiKey_RightShift;
        case GLFW_KEY_RIGHT_CONTROL:
            return ImGuiKey_RightCtrl;
        case GLFW_KEY_RIGHT_ALT:
            return ImGuiKey_RightAlt;
        case GLFW_KEY_RIGHT_SUPER:
            return ImGuiKey_RightSuper;
        case GLFW_KEY_MENU:
            return ImGuiKey_Menu;
        case GLFW_KEY_0:
            return ImGuiKey_0;
        case GLFW_KEY_1:
            return ImGuiKey_1;
        case GLFW_KEY_2:
            return ImGuiKey_2;
        case GLFW_KEY_3:
            return ImGuiKey_3;
        case GLFW_KEY_4:
            return ImGuiKey_4;
        case GLFW_KEY_5:
            return ImGuiKey_5;
        case GLFW_KEY_6:
            return ImGuiKey_6;
        case GLFW_KEY_7:
            return ImGuiKey_7;
        case GLFW_KEY_8:
            return ImGuiKey_8;
        case GLFW_KEY_9:
            return ImGuiKey_9;
        case GLFW_KEY_A:
            return ImGuiKey_A;
        case GLFW_KEY_B:
            return ImGuiKey_B;
        case GLFW_KEY_C:
            return ImGuiKey_C;
        case GLFW_KEY_D:
            return ImGuiKey_D;
        case GLFW_KEY_E:
            return ImGuiKey_E;
        case GLFW_KEY_F:
            return ImGuiKey_F;
        case GLFW_KEY_G:
            return ImGuiKey_G;
        case GLFW_KEY_H:
            return ImGuiKey_H;
        case GLFW_KEY_I:
            return ImGuiKey_I;
        case GLFW_KEY_J:
            return ImGuiKey_J;
        case GLFW_KEY_K:
            return ImGuiKey_K;
        case GLFW_KEY_L:
            return ImGuiKey_L;
        case GLFW_KEY_M:
            return ImGuiKey_M;
        case GLFW_KEY_N:
            return ImGuiKey_N;
        case GLFW_KEY_O:
            return ImGuiKey_O;
        case GLFW_KEY_P:
            return ImGuiKey_P;
        case GLFW_KEY_Q:
            return ImGuiKey_Q;
        case GLFW_KEY_R:
            return ImGuiKey_R;
        case GLFW_KEY_S:
            return ImGuiKey_S;
        case GLFW_KEY_T:
            return ImGuiKey_T;
        case GLFW_KEY_U:
            return ImGuiKey_U;
        case GLFW_KEY_V:
            return ImGuiKey_V;
        case GLFW_KEY_W:
            return ImGuiKey_W;
        case GLFW_KEY_X:
            return ImGuiKey_X;
        case GLFW_KEY_Y:
            return ImGuiKey_Y;
        case GLFW_KEY_Z:
            return ImGuiKey_Z;
        case GLFW_KEY_F1:
            return ImGuiKey_F1;
        case GLFW_KEY_F2:
            return ImGuiKey_F2;
        case GLFW_KEY_F3:
            return ImGuiKey_F3;
        case GLFW_KEY_F4:
            return ImGuiKey_F4;
        case GLFW_KEY_F5:
            return ImGuiKey_F5;
        case GLFW_KEY_F6:
            return ImGuiKey_F6;
        case GLFW_KEY_F7:
            return ImGuiKey_F7;
        case GLFW_KEY_F8:
            return ImGuiKey_F8;
        case GLFW_KEY_F9:
            return ImGuiKey_F9;
        case GLFW_KEY_F10:
            return ImGuiKey_F10;
        case GLFW_KEY_F11:
            return ImGuiKey_F11;
        case GLFW_KEY_F12:
            return ImGuiKey_F12;
        case GLFW_KEY_F13:
            return ImGuiKey_F13;
        case GLFW_KEY_F14:
            return ImGuiKey_F14;
        case GLFW_KEY_F15:
            return ImGuiKey_F15;
        case GLFW_KEY_F16:
            return ImGuiKey_F16;
        case GLFW_KEY_F17:
            return ImGuiKey_F17;
        case GLFW_KEY_F18:
            return ImGuiKey_F18;
        case GLFW_KEY_F19:
            return ImGuiKey_F19;
        case GLFW_KEY_F20:
            return ImGuiKey_F20;
        case GLFW_KEY_F21:
            return ImGuiKey_F21;
        case GLFW_KEY_F22:
            return ImGuiKey_F22;
        case GLFW_KEY_F23:
            return ImGuiKey_F23;
        case GLFW_KEY_F24:
            return ImGuiKey_F24;
        default:
            return ImGuiKey_None;
    }
}