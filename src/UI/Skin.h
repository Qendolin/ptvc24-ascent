#pragma once

#include <vector>

#include "UI.h"

namespace gl {
class GLObject;
}

namespace ui {
struct Skin {
    ~Skin();

    // all opengl resources that need to be destroyed when the skin is deleted
    std::vector<gl::GLObject*> resources;

    struct nk_nine_slice progressNormalBackground;
    struct nk_nine_slice progressNormalCursor;

    struct nk_nine_slice buttonNormalBackground;
    struct nk_nine_slice buttonHoverBackground;
    struct nk_nine_slice buttonActiveBackground;
    struct nk_color buttonNormalText;
    struct nk_color buttonHoverText;
    struct nk_color buttonActiveText;

    void apply(nk_context* nk);
};

Skin* loadSkin();

}  // namespace ui
