#pragma once

#include <array>

struct DebugSettings {
    struct Entity {
        bool debugDrawEnabled = false;
    };
    struct Rendering {
        bool normalMapsEnabled = true;

        struct Bloom {
            float factor = 1.0f;
            std::array<float, 6> levels = {1.0f, 0.5f, 0.25f, 0.125f, 0.0625f, 0.03125f};
            float threshold = 1.0f;
            float thresholdKnee = 0.6f;
        } bloom;
    };
    Entity entity;
    Rendering rendering;
    bool freeCam = false;
};
