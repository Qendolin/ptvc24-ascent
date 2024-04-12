#include "MainMenu.h"

#include "../../GL/Texture.h"
#include "../../Game.h"
#include "../../Input.h"
#include "../../Loader/Loader.h"
#include "../../Utils.h"
#include "../../Window.h"
#include "../UI.h"

MainMenuScreen::MainMenuScreen() {
    titleImage = loader::texture("assets/textures/ui/title.png", {.srgb = true});
}

MainMenuScreen::~MainMenuScreen() {
    delete titleImage;
}

void MainMenuScreen::draw() {
    using namespace ui::literals;

    Game& game = Game::get();
    nk_context* nk = game.ui->context();

    // transparent background
    nk->style.window.background = nk_rgba(0, 0, 0, 0);
    nk->style.window.fixed_background = nk_style_item_color(nk_rgba(0, 0, 0, 0));

    if (nk_begin(nk, "main_menu", {30_vw, 10_vh, 40_vw, 80_vh}, 0)) {
        // draw title image
        float img_height = 40_vw * titleImage->height() / titleImage->width();
        nk_layout_row_dynamic(nk, img_height, 1);
        nk_image_color(nk, nk_image_id(titleImage->id()), nk_rgba_f(1.0, 1.0, 1.0, titleOpacity.peek()));
        game.tween->step(titleOpacity);

        // vertil spacer
        nk_layout_row_dynamic(nk, img_height / 2, 1);

        // prepare sublayout for buttons
        auto used = nk_widget_position(nk);
        auto avail = nk_window_get_content_region_size(nk);
        nk_layout_row_begin(nk, NK_STATIC, avail.y - used.y, 2);
        nk_layout_row_push(nk, (40_vw - 240_dp) / 2.0f);
        nk_spacer(nk);
        nk_layout_row_push(nk, 240_dp);

        // draw menu buttons
        if (nk_group_begin(nk, "menu_buttons", 0)) {
            // row spacing
            nk_style_push_float(nk, &nk->style.window.spacing.y, 50_dp);

            nk_layout_row_dynamic(nk, 60_dp, 1);
            nk_style_set_font(nk, &game.ui->fonts()->get("menu_md")->handle);
            if (nk_button_label(nk, "Play")) {
                game.input->captureMouse();
                close();
            }
            if (nk_button_label(nk, "Settings")) {
                LOG("Settings pressed");
            }
            if (nk_button_label(nk, "Quit")) {
                glfwSetWindowShouldClose(game.window, true);
            }

            nk_style_pop_float(nk);  // row sapcing
        }
    }

    nk_end(nk);
}