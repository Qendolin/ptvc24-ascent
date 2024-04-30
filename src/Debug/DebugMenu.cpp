#include "DebugMenu.h"

#include <limits>
#include <string>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

#include "../Game.h"
#include "../Input.h"
#include "../Physics/Physics.h"
#include "Settings.h"

DebugMenu::DebugMenu() {
    frameTimes.nextMin = std::numeric_limits<float>::infinity();
    frameTimes.nextMin = -std::numeric_limits<float>::infinity();
}

DebugMenu::~DebugMenu() = default;

void DebugMenu::draw() {
    if (!open) return;

    drawDebugWindow_();
    drawPerformanceWindow_();
}

void DebugMenu::drawDebugWindow_() {
    using namespace ImGui;

    DebugSettings& settings = Game::get().debugSettings;

    ph::Physics& physics = *Game::get().physics;
    Begin("Debug Menu", nullptr, 0);

    // Misc
    Checkbox("Free Cam", &settings.freeCam);

    // Physics
    if (CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen)) {
        PushID("physics");
        Indent();
        bool physics_enabled = physics.enabled();
        if (Checkbox("Enable", &physics_enabled)) {
            physics.setEnabled(physics_enabled);
        }
        bool debug_draw_enabled = physics.debugDrawEnabled();
        if (Checkbox("Debug Draw", &debug_draw_enabled)) {
            physics.setDebugDrawEnabled(debug_draw_enabled);
        }
        Unindent();
        PopID();
    }

    // Entities
    if (CollapsingHeader("Entities", ImGuiTreeNodeFlags_DefaultOpen)) {
        PushID("entities");
        Indent();
        Checkbox("Debug Draw", &settings.entity.debugDrawEnabled);
        Unindent();
        PopID();
    }

    // Rendering
    if (CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen)) {
        PushID("rendering");
        Indent();
        Checkbox("Normal Mapping", &settings.rendering.normalMapsEnabled);

        if (CollapsingHeader("Bloom", ImGuiTreeNodeFlags_DefaultOpen)) {
            SliderFloat("Factor", &settings.rendering.bloom.factor, 0.0f, 1.0f);
            SliderFloat("Bloom Threshold", &settings.rendering.bloom.threshold, 0.0f, 5.0f);
            SliderFloat("Bloom Knee", &settings.rendering.bloom.thresholdKnee, 0.0f, 1.0f);

            for (int i = 0; i < settings.rendering.bloom.levels.size(); i++) {
                SliderFloat(("Level " + std::to_string(i)).c_str(), &settings.rendering.bloom.levels[i], 0.0f, 1.0f);
            }
        }

        if (CollapsingHeader("Lens Effects", ImGuiTreeNodeFlags_DefaultOpen)) {
            PushID("lens_effects");
            SliderFloat("Factor##factor", &settings.rendering.lens.factor, 0.0f, 1.0f);
            SliderFloat("Chromatic Distortion##chromaticDistortion", &settings.rendering.lens.chromaticDistortion, 0.0f, 4.0f);
            Checkbox("Blur##blur", &settings.rendering.lens.blur);

            Text("Ghosts");
            SliderInt("Count##ghosts", &settings.rendering.lens.ghosts, 0, 10);
            SliderFloat("Dispersion##ghostDispersion", &settings.rendering.lens.ghostDispersion, 0.0f, 1.0f);
            SliderFloat("Bias##ghostBias", &settings.rendering.lens.ghostBias, -100.0f, 0.0f);
            SliderFloat("Factor##ghostFactor", &settings.rendering.lens.ghostFactor, 0.0f, 1.0f);

            Text("Halo");
            SliderFloat("Size##haloSize", &settings.rendering.lens.haloSize, 0.0f, 1.0f);
            SliderFloat("Bias##haloBias", &settings.rendering.lens.haloBias, -100.0f, 0.0f);
            SliderFloat("Factor##haloFactor", &settings.rendering.lens.haloFactor, 0.0f, 1.0f);
            Text("Glare");
            SliderFloat("Attenuation##glareAttenuation", &settings.rendering.lens.glareAttenuation, 0.0f, 1.0f);
            SliderFloat("Bias##glareBias", &settings.rendering.lens.glareBias, -100.0f, 0.0f);
            SliderFloat("Factor##glareFactor", &settings.rendering.lens.glareFactor, 0.0f, 1.0f);

            PopID();
        }

        if (CollapsingHeader("Vignette", ImGuiTreeNodeFlags_DefaultOpen)) {
            PushID("vignette");
            SliderFloat("Factor", &settings.rendering.vignette.factor, 0.0f, 5.0f);
            SliderFloat("Inner", &settings.rendering.vignette.inner, 0.0f, 1.0f);
            SliderFloat("Outer", &settings.rendering.vignette.outer, 0.0f, 2.0f);
            SliderFloat("Sharpness", &settings.rendering.vignette.sharpness, 0.5f, 16.0f);
            PopID();
        }

        Unindent();
        PopID();
    }

    End();
}

void DebugMenu::drawPerformanceWindow_() {
    using namespace ImGui;

    Input& input = *Game::get().input;

    Begin("Performance", nullptr, 0);
    frameTimes.update(input.timeDelta());

    const auto sixty_fps_line_color = 0x808000ff;

    auto draw_list = GetWindowDrawList();
    Text("%4d fps", frameTimes.current <= 0.00001f ? 0 : (int)(1.0f / frameTimes.current));
    frameTimes.single[frameTimes.singleIndex] = frameTimes.current * 1000;
    frameTimes.singleIndex = (frameTimes.singleIndex + 1) % frameTimes.single.size();
    auto sixty_fps_line_point = GetCursorScreenPos() + ImVec2{0, 48};
    std::string frame_time_text = std::format("Frame Time - {:4.1f} ms", frameTimes.current * 1000);
    PlotHistogram("", &frameTimes.single.front(), (int)frameTimes.single.size(), frameTimes.singleIndex, frame_time_text.c_str(), 0.0f, 1000.0f / 30.0f, ImVec2{256, 96});
    draw_list->AddLine(sixty_fps_line_point, sixty_fps_line_point + ImVec2{256, 0}, sixty_fps_line_color);

    sixty_fps_line_point = GetCursorScreenPos() + ImVec2{0, 48};
    std::string frame_avg_text = std::format("Avg. Frame Time - {:4.1f} ms", frameTimes.currentAvg * 1000);
    PlotLines("", &frameTimes.avg.front(), frameTimes.avg.size(), frameTimes.cumulativeIndex, frame_avg_text.c_str(), 0, 1000.0f / 30.0f, ImVec2{256, 96});
    draw_list->AddLine(sixty_fps_line_point, sixty_fps_line_point + ImVec2{256, 0}, sixty_fps_line_color);

    sixty_fps_line_point = GetCursorScreenPos() + ImVec2{0, 48};
    std::string frame_min_text = std::format("Min. Frame Time - {:4.1f} ms", frameTimes.currentMin * 1000);
    PlotLines("", &frameTimes.min.front(), frameTimes.min.size(), frameTimes.cumulativeIndex, frame_min_text.c_str(), 0, 1000.0f / 30.0f, ImVec2{256, 96});
    draw_list->AddLine(sixty_fps_line_point, sixty_fps_line_point + ImVec2{256, 0}, sixty_fps_line_color);

    sixty_fps_line_point = GetCursorScreenPos() + ImVec2{0, 48};
    std::string frame_max_text = std::format("Max. Frame Time - {:4.1f} ms", frameTimes.currentMax * 1000);
    PlotLines("", &frameTimes.max.front(), frameTimes.max.size(), frameTimes.cumulativeIndex, frame_max_text.c_str(), 0, 1000.0f / 30.0f, ImVec2{256, 96});
    draw_list->AddLine(sixty_fps_line_point, sixty_fps_line_point + ImVec2{256, 0}, sixty_fps_line_color);

    End();
}

void DebugMenu::FrameTimes::update(float delta) {
    current = delta;
    if (current < nextMin)
        nextMin = current;
    if (current > nextMax)
        nextMax = current;

    nextAvgSum += 1;
    nextAvgTimer += current;

    if (nextAvgTimer >= 1.0) {
        currentAvg = nextAvgTimer / (float)nextAvgSum;
        currentMin = isinf(nextMin) ? 0 : nextMin;
        currentMax = isinf(nextMax) ? 0 : nextMax;

        avg[cumulativeIndex] = currentAvg * 1000;
        min[cumulativeIndex] = currentMin * 1000;
        max[cumulativeIndex] = currentMax * 1000;

        cumulativeIndex = (cumulativeIndex + 1) % avg.size();
        nextMin = std::numeric_limits<float>::infinity();
        nextMax = -std::numeric_limits<float>::infinity();
        nextAvgTimer = 0;
        nextAvgSum = 0;
    }
}