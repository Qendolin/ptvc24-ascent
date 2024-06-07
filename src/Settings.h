#pragma once

#include <string>

struct Settings {
    // vertical fov in deg
    float fov = 110;

    // How much the camera turns when moving the mouse. The unit is Degrees / Pixel.
    float lookSensitivity = 0.2f;

    float maxFps = 120.0f;
};

class SettingsManager {
    std::string filename_;
    Settings settings_;

   public:
    SettingsManager(std::string filename);

    void load();
    void save();

    Settings get() {
        return settings_;
    }

    void set(Settings settings) {
        settings_ = settings;
    }
};