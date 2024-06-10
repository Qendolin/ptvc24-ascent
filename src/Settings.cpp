#include "Settings.h"

#include <tortellini.h>

#include <filesystem>
#include <fstream>

#include "Util/Log.h"

SettingsManager::SettingsManager(std::string filename) : filename_(filename) {
}

void SettingsManager::save() {
    LOG_INFO("Saving settings");
    tortellini::ini ini;
    const auto &section = ini["settings"];
    section["fov"] = settings_.fov;
    section["look_sensitivity"] = settings_.lookSensitivity;
    section["max_fps"] = settings_.maxFps;
    section["motion_blur"] = settings_.motionBlur;
    section["gtao"] = settings_.gtao;
    section["master_volume"] = settings_.masterVolume;
    section["sound_volume"] = settings_.soundVolume;
    section["music_volume"] = settings_.musicVolume;

    std::fstream file = std::fstream(filename_, std::ios::out | std::ios::trunc);
    file << ini;
    file.close();
}

void SettingsManager::load() {
    LOG_INFO("Loading settings");
    // reset
    settings_ = Settings{};

    if (!std::filesystem::exists(filename_)) {
        return;
    }

    tortellini::ini ini;
    std::fstream file = std::fstream(filename_, std::ios::in);
    file >> ini;
    file.close();

    const auto &section = ini["settings"];
    settings_.fov = section["fov"] | settings_.fov;
    settings_.lookSensitivity = section["look_sensitivity"] | settings_.lookSensitivity;
    settings_.maxFps = section["max_fps"] | settings_.maxFps;
    settings_.motionBlur = section["motion_blur"] | settings_.motionBlur;
    settings_.gtao = section["gtao"] | settings_.gtao;
    settings_.masterVolume = section["master_volume"] | settings_.masterVolume;
    settings_.soundVolume = section["sound_volume"] | settings_.soundVolume;
    settings_.musicVolume = section["music_volume"] | settings_.musicVolume;
}