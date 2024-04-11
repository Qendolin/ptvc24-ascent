#pragma once

#include "../Screen.h"

class DebugMenuScreen : public Screen {
   public:
    DebugMenuScreen() {}
    ~DebugMenuScreen() {}

    void draw() override;
};