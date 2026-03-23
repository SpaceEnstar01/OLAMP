#pragma once

#include "lamp_mechanism.h"

// High-level semantic action types; intended to be used by the
// server-side (xiaozhi-server) or on-device intent dispatcher.
enum class LampActionType {
    kUnknown = 0,
    kBaseYawRelative,       // 底座左右转 (joint 0)
    kWristPitchRelative,    // 灯头抬头/低头 (joint 4)
    kGoPreset,              // 跳到一个命名预设姿态
};

// Intent-layer command: after intent parsing, before mapping to concrete angles.
// (Execution-layer command with preset/angles is in lamp_mapping.h as LampCommand.)
struct LampIntentCommand {
    LampActionType type = LampActionType::kUnknown;

    // For relative joint moves:
    int joint_index = -1;      // 0..4
    float delta_deg = 0.0f;    // 相对角度变化
    float speed_deg_per_s = 0.0f;

    // For presets:
    const char* preset_name = nullptr;
};

// Given a parsed intent label (e.g. "turn_left_base", "head_up_small"),
// fill a LampIntentCommand describing which joint to move and by how much.
// This header only defines the struct; actual NLU/keyword mapping logic
// should live on the server side or in a higher layer.
//
// Here we only provide minimal helpers to map common joint roles.

// Helper to get joint info for base_yaw / wrist_pitch etc.
const LampJointInfo* GetBaseYawJoint();
const LampJointInfo* GetWristPitchJoint();

