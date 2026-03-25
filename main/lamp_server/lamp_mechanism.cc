#include "lamp_mechanism.h"

#include <string.h>

#include "servo/calibration/lamp0324.h"

// NOTE:
// This file provides a semantic description of the current lamp model
// using the calibration data (lamp0324.h, lamp03.h, etc.).
//
// Angle ranges are derived from the same register ranges that MultiServo
// uses, so when you switch calibration header (lamp0324 -> lamp03), the
// semantic description will automatically follow.

static LampJointInfo g_joints[5];
static bool g_initialized = false;

static void EnsureLampMechanismInitialized() {
    if (g_initialized) {
        return;
    }

    // Helper to compute approximate logical angle from raw register range.
    auto compute_angle_range = [](int idx, float& out_min_deg, float& out_max_deg) {
        float min_raw = static_cast<float>(servo_range_min[idx]);
        float max_raw = static_cast<float>(servo_range_max[idx]);
        float mid = (min_raw + max_raw) / 2.0f;
        out_min_deg = (min_raw - mid) * 360.0f / 4095.0f;
        out_max_deg = (max_raw - mid) * 360.0f / 4095.0f;
    };

    for (int i = 0; i < 5; ++i) {
        float ang_min = 0.0f;
        float ang_max = 0.0f;
        compute_angle_range(i, ang_min, ang_max);

        LampJointInfo info{};
        info.joint_index = i;
        info.servo_id = i + 1;  // lamp calibration comment: base_yaw(1)..wrist_pitch(5)
        info.angle_min_deg = ang_min;
        info.angle_max_deg = ang_max;

        switch (i) {
        case 0:
            info.name = "base_yaw";
            info.human_name = "底座旋转";
            info.function = "控制整个台灯左右转向";
            info.motion_type = "rotation";
            info.axis = "yaw";
            info.positive_meaning = "向右转";
            info.negative_meaning = "向左转";
            break;
        case 1:
            info.name = "base_pitch";
            info.human_name = "主灯臂俯仰";
            info.function = "抬起或放下整个灯臂";
            info.motion_type = "pitch";
            info.axis = "pitch";
            info.positive_meaning = "向上抬高";
            info.negative_meaning = "向下放低";
            break;
        case 2:
            info.name = "elbow_pitch";
            info.human_name = "肘部俯仰";
            info.function = "折叠或伸展灯臂";
            info.motion_type = "pitch";
            info.axis = "pitch";
            info.positive_meaning = "展开伸直";
            info.negative_meaning = "收起折叠";
            break;
        case 3:
            info.name = "wrist_roll";
            info.human_name = "灯头扭转";
            info.function = "旋转灯头朝向，不改变俯仰";
            info.motion_type = "roll";
            info.axis = "roll";
            info.positive_meaning = "顺时针扭转";
            info.negative_meaning = "逆时针扭转";
            break;
        case 4:
            info.name = "wrist_pitch";
            info.human_name = "灯头俯仰";
            info.function = "灯头抬头、低头、照射方向上下调整";
            info.motion_type = "pitch";
            info.axis = "pitch";
            info.positive_meaning = "抬头向上看";
            info.negative_meaning = "低头向下看";
            break;
        default:
            break;
        }

        g_joints[i] = info;
    }

    g_initialized = true;
}

const LampJointInfo* GetLampJoints(int& count) {
    EnsureLampMechanismInitialized();
    count = 5;
    return g_joints;
}

const LampJointInfo* FindLampJointByName(const char* name) {
    if (!name) {
        return nullptr;
    }
    int count = 0;
    const LampJointInfo* joints = GetLampJoints(count);
    for (int i = 0; i < count; ++i) {
        if (joints[i].name && strcmp(joints[i].name, name) == 0) {
            return &joints[i];
        }
    }
    return nullptr;
}

