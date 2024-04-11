#pragma once

#include <array>

// Not actually a screen
class DebugMenu {
    struct FrameTimes {
        int singleIndex = 0;
        int cumulativeIndex = 0;
        // current frame time in seconds
        float current = 0;
        float currentMin = 0;
        float currentMax = 0;
        float currentAvg = 0;

        float nextMin;
        float nextMax;
        int nextAvgSum = 0;
        float nextAvgTimer = 0;
        // history frame times in ms
        std::array<float, 128> single = {};
        std::array<float, 32> avg = {};
        std::array<float, 32> min = {};
        std::array<float, 32> max = {};

        void update(float delta);
    } frameTimes;

    void drawDebugWindow_();
    void drawPerformanceWindow_();

   public:
    // `true` will draw the debug menu. `false` will hide it
    bool open = false;
    DebugMenu();
    ~DebugMenu() {}

    void draw();
};