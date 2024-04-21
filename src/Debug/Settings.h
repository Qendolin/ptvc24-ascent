#pragma once

struct DebugSettings {
    struct Entity {
        bool debugDrawEnabled = false;
    };
    struct Rendering {
        bool normalMapsEnabled = true;
    };
    Entity entity;
    Rendering rendering;
    bool freeCam = false;
};
