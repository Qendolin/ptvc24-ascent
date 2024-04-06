#include "Gltf.h"

#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>

namespace gltf = tinygltf;

using namespace Asset;

namespace Loader {

const gltf::Model gltf(const std::string filename) {
    gltf::TinyGLTF loader;
    gltf::Model model;
    std::string err;
    std::string warn;

    LOG("Loading GLTF: " + filename);

    std::string ext = filename.substr(filename.find_last_of("."));
    bool ok = false;
    if (ext == ".glb") {
        ok = loader.LoadBinaryFromFile(&model, &err, &warn, filename);
    } else if (ext == ".gltf") {
        ok = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
    } else {
        PANIC("Unknown gltf file extension '" + ext + "', should be .gltf or .glb");
    }

    if (!warn.empty()) {
        LOG("Warning: " + warn);
    }

    if (!err.empty()) {
        LOG("Error: " + err);
    }

    if (!ok) {
        PANIC("Failed to load glTF: " + filename);
    }
    return model;
}

Scene *scene(const gltf::Model &model) {
    Graphics g = graphics(model);
    Physics ph = physics(model);
    std::string name = model.scenes[model.defaultScene].name;
    return new Scene(name, std::move(g), std::move(ph));
}

}  // namespace Loader