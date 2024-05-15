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

        struct Lens {
            float factor = 0.3f;
            float chromaticDistortion = 1.2f;
            bool blur = true;

            int ghosts = 6;
            float ghostDispersion = 0.925f;
            float ghostBias = -20;
            float ghostFactor = 0.25f;

            float haloSize = 0.45f;
            float haloBias = -10;
            float haloFactor = 0.05f;

            float glareBias = -3.0f;
            float glareFactor = 0.05f;
            float glareAttenuation = 0.91f;
        } lens;

        struct Vignette {
            float factor = 0.8f;
            float inner = 0.6f;
            float outer = 1.6f;
            float sharpness = 2.0f;
        } vignette;

        struct Shadow {
            bool debugDrawEnabled = false;
            std::array<float, 3> sunTarget = {-27, 0, -70};
            std::array<float, 2> sunAzimuthElevation = {55.8f, 48};
            float sunDistance = 175;
            float normalBias = 0.035f;
            float depthBias = 0.0f;
            float offsetFactor = 1.85f;
            float offsetUnits = 4.5f;
            float offsetClamp = 0.05f;
        } shadow;
    };
    Entity entity;
    Rendering rendering;
    bool freeCam = false;
};
