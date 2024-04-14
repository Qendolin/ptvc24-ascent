#include "GL/StateManager.h"
#include "GL/Util.h"
#include "Game.h"
#include "Setup.h"
#include "Utils.h"
#include "Window.h"

void printNotDeletedOpenGLObjects() {
    std::vector<std::pair<GLenum, GLuint>> tracked = gl::manager->tracked();
    if (tracked.empty()) return;

    std::cerr << "Some OpenGL Object were not deleted!" << std::endl;
    for (const auto& [type, id] : tracked) {
        std::cerr << " - " << gl::getObjectNamespaceString(type) << " with id=" << id;

        std::string label = gl::getObjectLabel(type, id);
        if (!label.empty()) {
            std::cerr << ", label='" << label << "'";
        }

        std::cerr << std::endl;
    }
}

int main(int argc, char** argv) {
    // Print out the date and time of when this binary was built.
    // Note: Local time, not UTC.

#ifndef NDEBUG
    LOG_INFO("Debug build from " << __TIMESTAMP__);
#else
    LOG_INFO("Release build from " << __TIMESTAMP__);
#endif

    LOG_INFO("Parsing arguments");
    bool enableCompatibilityProfile = false;
    bool enableGlDebug = false;
    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);
        if (arg == "--enable-compatibility-profile") {
            enableCompatibilityProfile = true;
        }
        if (arg == "--enable-gl-debug") {
            enableGlDebug = true;
        }
    }

#ifndef NDEBUG
    enableGlDebug = true;
#endif

    try {
        Window window = createOpenGLContext(enableCompatibilityProfile);
        initializeOpenGL(enableGlDebug);

        Game* game = new Game(window);
        game->load();
        game->run();
        game->unload();
        delete game;

        printNotDeletedOpenGLObjects();

        destroyOpenGLContext(window);
    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::flush;
        return EXIT_FAILURE;
    }

    std::cerr << std::flush;
    LOG_INFO("Exit");
}
