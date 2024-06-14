#include "Object.h"

#include <utility>

#include "StateManager.h"
#include "Util.h"

namespace gl {

GLObject::GLObject(GLObject&& other) : type_(other.type_), id_(std::exchange(other.id_, 0)) {}

GLObject::~GLObject() {
    if (id_ == 0) return;

    std::string label = gl::getObjectLabel(type_, id_);

    std::string labelString = "";
    if (label.empty()) {
        labelString = " label='" + label + "'";
    }

    LOG_WARN("GL " << gl::getObjectNamespaceString(type_) << " Object id=" << std::to_string(id_) << labelString << " not destroyed!");
}

void GLObject::setDebugLabel(const std::string& label) {
    debugLabel_ = label;
    glObjectLabel(type_, id_, -1, label.c_str());
}

void GLObject::track_() {
    gl::manager->track(type_, id_);
}

void GLObject::untrack_() {
    gl::manager->untrack(type_, id_);
}

}  // namespace gl
