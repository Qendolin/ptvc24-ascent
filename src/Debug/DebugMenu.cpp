#include "DebugMenu.h"

#include <limits>
#include <string>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

#include <glm/gtc/type_ptr.hpp>

#include "../Game.h"
#include "../Input.h"
#include "../Particles/ParticleSystem.h"
#include "../Physics/Physics.h"
#include "Direct.h"
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
    drawParticlesWindow_();
}

void DebugMenu::drawDebugWindow_() {
    using namespace ImGui;

    Game& game = Game::get();
    DebugSettings& settings = game.debugSettings;

    ph::Physics& physics = *game.physics;
    Begin("Debug Menu", nullptr, 0);

    // Misc
    Checkbox("Free Cam", &settings.freeCam);
    LabelText("Position", "%.1f / %.1f / %.1f", game.camera->position.x, game.camera->position.y, game.camera->position.z);

    // Physics
    if (CollapsingHeader("Physics", 0)) {
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
    if (CollapsingHeader("Entities", 0)) {
        PushID("entities");
        Indent();
        Checkbox("Debug Draw", &settings.entity.debugDrawEnabled);
        Unindent();
        PopID();
    }

    // Rendering
    if (CollapsingHeader("Rendering")) {
        PushID("rendering");
        Indent();
        Checkbox("Normal Mapping", &settings.rendering.normalMapsEnabled);

        if (CollapsingHeader("Bloom")) {
            SliderFloat("Factor", &settings.rendering.bloom.factor, 0.0f, 1.0f);
            SliderFloat("Bloom Threshold", &settings.rendering.bloom.threshold, 0.0f, 5.0f);
            SliderFloat("Bloom Knee", &settings.rendering.bloom.thresholdKnee, 0.0f, 1.0f);

            for (int i = 0; i < settings.rendering.bloom.levels.size(); i++) {
                SliderFloat(("Level " + std::to_string(i)).c_str(), &settings.rendering.bloom.levels[i], 0.0f, 1.0f);
            }
        }

        if (CollapsingHeader("Lens Effects")) {
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

        if (CollapsingHeader("Vignette")) {
            PushID("vignette");
            SliderFloat("Factor", &settings.rendering.vignette.factor, 0.0f, 5.0f);
            SliderFloat("Inner", &settings.rendering.vignette.inner, 0.0f, 1.0f);
            SliderFloat("Outer", &settings.rendering.vignette.outer, 0.0f, 2.0f);
            SliderFloat("Sharpness", &settings.rendering.vignette.sharpness, 0.5f, 16.0f);
            PopID();
        }

        if (CollapsingHeader("Sun")) {
            PushID("sun");
            BeginTable("dir_input", 2);
            TableNextColumn();
            SliderFloat("Az", &settings.rendering.sun.azimuth, 0, 360, "%.1f °");
            TableNextColumn();
            SliderFloat("El", &settings.rendering.sun.elevation, -90, 90, "%.1f °");
            EndTable();
            ColorEdit3("Color", glm::value_ptr(settings.rendering.sun.color), ImGuiColorEditFlags_Float);
            SliderFloat("Brightness", &settings.rendering.sun.brightness, 0, 10);
            PopID();
        }

        if (CollapsingHeader("Shadow")) {
            PushID("shadow");
            Checkbox("Debug Draw", &settings.rendering.shadow.debugDrawEnabled);
            SliderFloat("Split Lambda", &settings.rendering.shadow.cascadeSplitLambda, 0.0, 1.0);
            DragFloat("Normal Bias", &settings.rendering.shadow.normalBias);
            SliderFloat("Size Bias", &settings.rendering.shadow.sizeBias, -300, 300);
            DragFloat("Depth Bias", &settings.rendering.shadow.depthBias);
            SliderFloat("Offset Factor", &settings.rendering.shadow.offsetFactor, -2.5f, 2.5f, "%.5f");
            DragFloat("Offset Units", &settings.rendering.shadow.offsetUnits);
            SliderFloat("Offset Clamp", &settings.rendering.shadow.offsetClamp, 0.0f, 0.1f, "%.5f");
            PopID();
        }

        if (CollapsingHeader("Terrain")) {
            PushID("terrain");
            Checkbox("Wireframe", &settings.rendering.terrain.wireframe);
            Checkbox("Debug LODs", &settings.rendering.terrain.fixedLodOrigin);
            PopID();
        }

        if (CollapsingHeader("GTAO")) {
            PushID("gtao");
            Checkbox("Enabled", &settings.rendering.ao.enabled);
            SliderFloat("Factor", &settings.rendering.ao.factor, 0.0, 1.0);
            SliderFloat("Radius", &settings.rendering.ao.radius, 0.0, 10.0);
            SliderFloat("Power", &settings.rendering.ao.power, 0.0, 10.0);
            PopID();
        }

        if (CollapsingHeader("Fog")) {
            PushID("fog");
            SliderFloat("Density", &settings.rendering.fog.density, 0.0, 0.1f, "%.5f");
            SliderFloat("Emission", &settings.rendering.fog.emission, 0.0, 0.1f, "%.5f");
            SliderFloat("Maximum", &settings.rendering.fog.maximum, 0.0, 1.0f);
            DragFloat("Height", &settings.rendering.fog.height);
            ColorEdit3("Color", glm::value_ptr(settings.rendering.fog.color), ImGuiColorEditFlags_Float);
            PopID();
        }

        Unindent();
        PopID();
    }

    End();
}

void DebugMenu::drawPerformanceWindow_() {
    using namespace ImGui;

    Game& game = Game::get();
    Input& input = *game.input;

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

void DebugMenu::drawParticlesWindow_() {
    using namespace ImGui;

    Game& game = Game::get();
    ParticleSystem& particles = *game.particles;
    Begin("Particles", nullptr, 0);

    // Misc
    LabelText("Reserved", "%d/%d", particles.reserved(), particles.capacity());

    auto format_system_name = [](ParticleEmitter& emitter) {
        glm::vec3 pos = emitter.settings().position;
        return std::format("{} at {:.0f} / {:.0f} / {:.0f}", emitter.material, pos.x, pos.y, pos.z);
    };

    auto emitters = particles.emitters();
    std::string selected_preview = "";
    if (state.particles.selected >= 0) {
        selected_preview = format_system_name(*emitters[state.particles.selected]);
    }
    if (BeginCombo("Systems", selected_preview.c_str(), 0)) {
        if (Selectable("None", state.particles.selected == -1)) {
            state.particles.selected = -1;
        }
        for (int i = 0; i < emitters.size(); i++) {
            std::string label = format_system_name(*emitters[i]);
            bool selected = state.particles.selected == i;
            if (Selectable(label.c_str(), selected)) {
                state.particles.selected = i;
            }
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        EndCombo();
    }

    if (state.particles.selected >= 0) {
        auto& emitter = *emitters[state.particles.selected];
        auto& settings = emitter.settings();
        game.directDraw->unshaded();
        game.directDraw->stroke(0.1f);
        game.directDraw->color(1, 0, 0);
        game.directDraw->circleLine(settings.position, settings.gravity, 2.5);
        game.directDraw->line(settings.position, settings.position + 5.0f * glm::normalize(settings.gravity));
        LabelText("Timer", "%.3f", emitter.intervalTimer());
        LabelText("Interval", "%.3f", emitter.nextInterval());
        Text("Settings");
        BeginDisabled(true);
        Range<int> count = settings.count;
        DragIntRange2("Count", &count.min, &count.max, 1, 0, 1000);
        Range<float> frequency = settings.frequency;
        DragFloatRange2("Frequency", &frequency.min, &frequency.max, 1, 0, 1000);
        Range<float> life = settings.life;
        DragFloatRange2("Life", &life.min, &life.max, 0.1f, 0, 60);
        EndDisabled();
        if (DragFloat3("Direction", glm::value_ptr(settings.direction), 0.1f)) {
            settings.direction = glm::normalize(settings.direction);
        }
        DragFloatRange2("Spread", &settings.spread.min, &settings.spread.max, 1, 0, 180);
        DragFloat3("Position", glm::value_ptr(settings.position));
        DragFloatRange2("Velocity", &settings.velocity.min, &settings.velocity.max, 0.5);
        glm::vec3 immutable_gravity = settings.gravity;
        DragFloat3("Gravity", glm::value_ptr(immutable_gravity), 0.1f, -100, 100);
        DragFloatRange2("Gravity Factor", &settings.gravityFactor.min, &settings.gravityFactor.max, 0.1f, -5, 5);
        DragFloatRange2("Drag", &settings.drag.min, &settings.drag.max, 0.0001f, 0, 1, "%.4f");
        DragFloatRange2("Rotation", &settings.rotation.min, &settings.rotation.max, 1, -360, 360);
        DragFloatRange2("Revolutions", &settings.revolutions.min, &settings.revolutions.max);
        DragFloat("Emissivity", &settings.emissivity, 0.1f, 0, 10);
        DragFloat2("Size", glm::value_ptr(settings.size), 0.01f);
        DragFloatRange2("Scale", &settings.scale.min, &settings.scale.max, 0.01f);
        DragFloat("Stretching", &settings.stretching, 0.1f, 0, 10);
    }

    End();
}