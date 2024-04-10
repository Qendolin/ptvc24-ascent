#include "Object.h"

#include "StateManager.h"
#include "Util.h"

namespace gl {

void GLObject::checkDestroyed() {
    if (id_ == 0) return;

    std::string label = gl::getObjectLabel(type_, id_);

    std::string labelString = "";
    if (label.empty()) {
        labelString = " label='" + label + "'";
    }

    std::cerr << "GL " << gl::getObjectNamespaceString(type_) << " Object id=" << std::to_string(id_) << labelString << " not destroyed!" << std::endl;
}

void GLObject::track_() {
    gl::manager->track(type_, id_);
}

void GLObject::untrack_() {
    gl::manager->untrack(type_, id_);
}

}  // namespace gl
