#include "lamp_presets.h"

#include <string.h>

#include "servo/calibration/lamp05.h"

// For now we only support lamp05 at compile time.
// In future, additional lampXX.h headers can be added here and
// dispatched by CALIBRATION_LAMP_ID.

static LampPreset g_lamp05_presets[kLamp05PresetCount];
static bool g_lamp05_initialized = false;

static void EnsureLamp05PresetsInitialized() {
    if (g_lamp05_initialized) {
        return;
    }

    for (int i = 0; i < kLamp05PresetCount; ++i) {
        g_lamp05_presets[i].model_id = CALIBRATION_LAMP_ID;
        g_lamp05_presets[i].name = kLamp05Presets[i].name;
        for (int j = 0; j < 5; ++j) {
            g_lamp05_presets[i].angles[j] = kLamp05Presets[i].angles[j];
        }
    }

    g_lamp05_initialized = true;
}

bool GetLampPresets(const LampPreset*& presets, int& count) {
    // Currently only lamp05 is compiled in.
    (void)presets;
    (void)count;

    EnsureLamp05PresetsInitialized();
    presets = g_lamp05_presets;
    count = kLamp05PresetCount;
    return true;
}

const LampPreset* FindLampPreset(const char* name) {
    if (!name) {
        return nullptr;
    }

    const LampPreset* presets = nullptr;
    int count = 0;
    if (!GetLampPresets(presets, count)) {
        return nullptr;
    }

    for (int i = 0; i < count; ++i) {
        if (presets[i].name && strcmp(presets[i].name, name) == 0) {
            return &presets[i];
        }
    }

    return nullptr;
}



