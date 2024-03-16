#include "Game.h"
#include "Setup.h"
#include "Utils.h"

int main(int argc, char** argv) {
    LOG("Build from " << __DATE__ << " " << __TIME__);

    LOG("Parsing arguments");
    bool enableCompatibilityProfile = false;
    bool disableGlDebug = false;
    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);
        if (arg == "--enable-compatibility-profile") {
            enableCompatibilityProfile = true;
        }
        if (arg == "--disable-gl-debug") {
            disableGlDebug = true;
        }
    }

    try {
        GLFWwindow* window = createOpenGLContext(enableCompatibilityProfile);
        initializeOpenGL(!disableGlDebug);

        Game* game = new Game(window);
        Game::instance = game;
        game->setup();
        game->run();

    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::flush;
        return EXIT_FAILURE;
    }

    std::cerr << std::flush;
    LOG("Exit");
}
