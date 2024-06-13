#pragma once

#include <array>

#include "../Scene/Light.h"

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

        OrthoLight sun = {
            .azimuth = 55.8f,
            .elevation = 48.0f,
            .color = glm::vec3{0.2f, 0.2f, 0.4f},
            .brightness = 5.5f,
        };

        struct Shadow {
            bool debugDrawEnabled = false;
            float cascadeSplitLambda = 0.75f;
            float normalBias = 300.0f;
            float sizeBias = 10.0f;
            float depthBias = 0.12f;
            float offsetFactor = 1.85f;
            float offsetUnits = 4.5f;
            float offsetClamp = 0.05f;
        } shadow;

        struct Terrain {
            bool wireframe = false;
            bool fixedLodOrigin = false;
        } terrain;

        struct Water {
            bool wireframe = false;
            bool fixedLodOrigin = false;
            float heightScale = 64.0;
        } water;

        struct AmbientOcclusion {
            bool enabled = true;
            float factor = 1.0;
            float radius = 1.0;
            float power = 2.0;
        } ao;

        struct Fog {
            float density = 0.0065f;
            float emission = 0.002f;
            float height = 20.0f;
            float maximum = 0.8f;
            glm::vec3 color = glm::vec3(83 / 255.0f, 110 / 255.0f, 170 / 255.0f);
        } fog;

        struct MotionBlur {
            float targetFps = 30.0;
        } motionBlur;
    };
    Entity entity;
    Rendering rendering;
    bool freeCam = false;
    bool infiniteBoost = false;
};
