#include "GL/StateManager.h"
#include "GL/Util.h"
#include "Game.h"
#include "Setup.h"
#include "Utils.h"

void printNotDeletedOpenGLObjects() {
    std::vector<std::pair<GLenum, GLuint>> tracked = GL::manager->tracked();
    if (tracked.empty()) return;

    std::cerr << "Some OpenGL Object were not deleted!" << std::endl;
    for (const auto& [type, id] : tracked) {
        std::cerr << " - " << GL::getObjectNamespaceString(type) << " with id=" << id;

        std::string label = GL::getObjectLabel(type, id);
        if (!label.empty()) {
            std::cerr << ", label='" << label << "'";
        }

        std::cerr << std::endl;
    }
}

int main(int argc, char** argv) {
    // Print out the date and time of when this binary was built.
    // Note: Local time, not UTC.
    LOG("Build from " << __TIMESTAMP__);

    LOG("Parsing arguments");
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
        GLFWwindow* window = createOpenGLContext(enableCompatibilityProfile);
        initializeOpenGL(enableGlDebug);

        Game* game = new Game(window);
        Game::instance = game;
        game->setup();
        game->run();
        delete game;

        printNotDeletedOpenGLObjects();

        glfwDestroyWindow(window);

    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::flush;
        return EXIT_FAILURE;
    }

    std::cerr << std::flush;
    LOG("Exit");
}
