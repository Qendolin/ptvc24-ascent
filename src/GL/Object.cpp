#include "Object.h"

#include "StateManager.h"
#include "Util.h"

namespace GL {

void GLObject::checkDestroyed() {
    if (id_ == 0) return;

    std::string label = GL::getObjectLabel(type_, id_);

    std::string labelString = "";
    if (label.empty()) {
        labelString = " label='" + label + "'";
    }

    std::cerr << "GL " << GL::getObjectNamespaceString(type_) << " Object id=" << std::to_string(id_) << labelString << " not destroyed!" << std::endl;
}

void GLObject::track_() {
    GL::manager->track(type_, id_);
}

void GLObject::untrack_() {
    GL::manager->untrack(type_, id_);
}

}  // namespace GL
