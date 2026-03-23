#include "lamp_mapping.h"

bool ResolvePresetCommand(LampCommand& cmd) {
    if (cmd.type != LampActionType::GO_PRESET || !cmd.preset_name) {
        return false;
    }

    const LampPreset* preset = FindLampPreset(cmd.preset_name);
    if (!preset) {
        return false;
    }

    for (int i = 0; i < 5; ++i) {
        cmd.angles[i] = preset->angles[i];
    }

    return true;
}


