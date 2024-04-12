#pragma once

#include <glm/glm.hpp>

// forward declaration
class Input;
namespace gl {
class Texture;
class VertexArray;
class ShaderPipeline;
class Buffer;
}  // namespace gl

namespace ui {

class ImGuiBackend {
   private:
    typedef int CallbackRegistrationID;

    bool hasCallbacksBound_ = false;
    CallbackRegistrationID keyCallbackID_ = 0;
    CallbackRegistrationID mousePosCallbackID_ = 0;
    CallbackRegistrationID mouseButtonCallbackID_ = 0;
    CallbackRegistrationID scrollCallbackID_ = 0;
    CallbackRegistrationID charCallbackID_ = 0;

    gl::Texture *fontAtlas_ = nullptr;
    gl::VertexArray *vao_ = nullptr;
    gl::Buffer *vbo_ = nullptr;
    gl::Buffer *ebo_ = nullptr;
    gl::ShaderPipeline *shader_ = nullptr;

    glm::mat4 projectionMatrix_ = glm::mat4(1.0);
    glm::ivec2 viewport_ = glm::ivec2(0, 0);
    double lastFrameTime_ = 0.0;

    bool enabled_ = true;

   public:
    static constexpr const char *INI_FILENAME = "local/imgui.ini";
    static constexpr const char *LOG_FILENAME = "local/imgui_log.txt";

    ImGuiBackend();

    ~ImGuiBackend();

    void update(Input &input);

    void bind(Input &input);

    bool shouldShowCursor();

    // must call unbind before the object is destroyed
    void unbind(Input &input);

    void setViewport(int width, int height);

    void render();
};

}  // namespace ui
