#pragma once
#include "Object.h"

namespace gl {
class Sync {
   private:
    bool init_ = false;
    GLsync sync_ = nullptr;

   public:
    /**
     * @param timeout wait timeout in ns
     * @returns `false` when the timeout expired or an error occured
     */
    bool clientWait(uint64_t timeout = 1000000000) {
        if (!init_) return true;
        GLenum status = glClientWaitSync(sync_, 0, timeout);
        bool ok = status == GL_ALREADY_SIGNALED || status == GL_CONDITION_SATISFIED;
        if (ok) {
            glDeleteSync(sync_);
            sync_ = nullptr;
            init_ = false;
        }
        return ok;
    }

    void fence() {
        sync_ = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        init_ = true;
    }
};
}  // namespace gl
