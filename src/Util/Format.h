#pragma once

#include <cmath>
#include <format>
#include <string>

static std::string formatTimeRaceClock(float total_seconds) {
    float minutes = std::floor(total_seconds / 60);
    float seconds = std::floor(total_seconds - minutes * 60);
    float millis = std::floor(1000 * (total_seconds - minutes * 60 - seconds));

    return std::format("{:02}:{:02}.{:03}", round(minutes), round(seconds), round(millis));
}