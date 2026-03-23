#pragma once

#include <stddef.h>

// High-level semantic description of a single lamp joint.
// Angles are logical degrees in the standard order:
// [base_yaw, base_pitch, elbow_pitch, wrist_roll, wrist_pitch].
struct LampJointInfo {
    int joint_index;        // 0..4 in logical order
    int servo_id;           // Physical servo ID on the bus (1..5)

    const char* name;       // Stable internal name, e.g. "base_yaw"
    const char* human_name; // Human-readable name, e.g. "底座旋转"

    const char* function;        // Short description of what this joint does
    const char* motion_type;     // e.g. "rotation", "pitch", "roll"
    const char* axis;            // e.g. "yaw", "pitch", "roll"
    const char* positive_meaning;// e.g. "向右转", "抬头向上看"
    const char* negative_meaning;// e.g. "向左转", "低头向下看"

    float angle_min_deg;    // Approximate min logical angle (deg)
    float angle_max_deg;    // Approximate max logical angle (deg)
};

// Get all joints for the currently compiled lamp model.
// The returned pointer is valid for the lifetime of the program.
// |count| will be set to the number of joints (typically 5).
const LampJointInfo* GetLampJoints(int& count);

// Find a joint by its internal name (e.g. "base_yaw").
// Returns nullptr if not found.
const LampJointInfo* FindLampJointByName(const char* name);

