#include "lamp_rules.h"

const LampJointInfo* GetBaseYawJoint() {
    int count = 0;
    const LampJointInfo* joints = GetLampJoints(count);
    for (int i = 0; i < count; ++i) {
        if (joints[i].joint_index == 0) {
            return &joints[i];
        }
    }
    return nullptr;
}

const LampJointInfo* GetWristPitchJoint() {
    int count = 0;
    const LampJointInfo* joints = GetLampJoints(count);
    for (int i = 0; i < count; ++i) {
        if (joints[i].joint_index == 4) {
            return &joints[i];
        }
    }
    return nullptr;
}

