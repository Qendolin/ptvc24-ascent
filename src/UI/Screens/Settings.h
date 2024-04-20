#pragma once

#include "../../Settings.h"
#include "../Screen.h"

class SettingsScreen : public Screen {
   public:
    Settings settings;

    SettingsScreen(Settings score);
    virtual ~SettingsScreen();

    void draw() override;
};