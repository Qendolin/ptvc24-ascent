#pragma once

#include "../../Settings.h"
#include "../Screen.h"

class SettingsScreen : public Screen {
   private:
    Settings settings_ = {};

    void draw_() override;

   public:
    SettingsScreen() = default;
    virtual ~SettingsScreen() = default;

    void open(Settings settings);
};