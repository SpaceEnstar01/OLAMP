#pragma once

#include <stddef.h>

// Generic lamp preset description, independent of concrete model.
// Angles are lerobot-compatible degrees in the order:
// [base_yaw, base_pitch, elbow_pitch, wrist_roll, wrist_pitch].
struct LampPreset {
    const char* model_id;  // e.g. "lamp05"
    const char* name;      // e.g. "home", "horizontal", "vertical"
    float angles[5];
};

// Get all presets for the currently compiled lamp model.
// Returns true on success and sets |presets| and |count|.
bool GetLampPresets(const LampPreset*& presets, int& count);

// Find a preset by name for the current lamp model.
// Returns nullptr if not found.
const LampPreset* FindLampPreset(const char* name);





