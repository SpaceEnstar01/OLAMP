#include "lamp_presets.h"

#include <string.h>

#include "servo/calibration/lamp0324.h"

// For now we only support lamp0324 at compile time.
// In future, additional lampXX.h headers can be added here and
// dispatched by CALIBRATION_LAMP_ID.

static LampPreset g_lamp_presets[kLamp0324PresetCount];
static bool g_lamp_presets_initialized = false;

static void EnsureLampPresetsInitialized() {
    if (g_lamp_presets_initialized) {
        return;
    }

    for (int i = 0; i < kLamp0324PresetCount; ++i) {
        g_lamp_presets[i].model_id = CALIBRATION_LAMP_ID;
        g_lamp_presets[i].name = kLamp0324Presets[i].name;
        for (int j = 0; j < 5; ++j) {
            g_lamp_presets[i].angles[j] = kLamp0324Presets[i].angles[j];
        }
    }

    g_lamp_presets_initialized = true;
}

bool GetLampPresets(const LampPreset*& presets, int& count) {
    // Currently only lamp0324 is compiled in.
    (void)presets;
    (void)count;

    EnsureLampPresetsInitialized();
    presets = g_lamp_presets;
    count = kLamp0324PresetCount;
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



