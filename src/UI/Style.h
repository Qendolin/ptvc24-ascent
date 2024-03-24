#pragma once

#include <nuklear.h>

#include <string>

typedef struct Skin {
    struct nk_nine_slice button_normal_background;
    struct nk_nine_slice button_hover_background;
    struct nk_nine_slice button_active_background;
    struct nk_color button_normal_text;
    struct nk_color button_hover_text;
    struct nk_color button_active_text;

    void apply(nk_context* nk);
} Skin;

Skin load_skin();