#pragma once

#include "lamp_presets.h"

// Minimal high-level action types supported initially.
enum class LampActionType {
    GO_PRESET,   // Move to a named preset
    SET_ANGLES,  // Directly set 5 target angles
};

// High-level command description passed within lamp_server.
struct LampCommand {
    LampActionType type;
    const char* preset_name;  // Used when type == GO_PRESET
    float angles[5];          // Used when type == SET_ANGLES or resolved preset
    int duration_ms;          // Total move duration in milliseconds
};

// Resolve a GO_PRESET command into concrete angles using lamp presets.
// Returns false if preset not found.
bool ResolvePresetCommand(LampCommand& cmd);


