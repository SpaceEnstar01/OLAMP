#pragma once

// High-level helpers for lamp control from MCP / application code.
// These functions will internally resolve presets (if needed) and
// call into ServoManager to execute smooth multi-servo moves.

// Move lamp to a named preset over the given duration.
// Returns false if preset not found or servo system not ready.
bool LampSendPreset(const char* name, int duration_ms);

// Directly move lamp to 5 target angles over the given duration.
// Angles are in degrees, order:
// [base_yaw, base_pitch, elbow_pitch, wrist_roll, wrist_pitch].
bool LampSendAngles(const float angles[5], int duration_ms);

// Relative move for joint 1 (base_yaw) based on current angle.
// delta_deg: relative delta in degrees (e.g. +10 for "右转一点点", -10 for "左转一点点").
// speed_deg_per_s: desired angular speed in deg/s (controls duration).
bool LampMoveJoint1Relative(float delta_deg, float speed_deg_per_s);

// Relative move for joint 2 (base_pitch/height) based on current cached angle.
// delta_deg: signed delta in degrees, +up / -down.
// speed_deg_per_s: desired angular speed in deg/s (controls duration).
bool LampMoveJoint2Relative(float delta_deg, float speed_deg_per_s);

// Relative move for joint 5 (wrist_pitch) based on current angle.
// delta_deg: relative delta in degrees (e.g. +10 for "抬高一点").
// speed_deg_per_s: desired angular speed in deg/s (controls duration).
// Returns false if servo system not ready.
bool LampMoveJoint5Relative(float delta_deg, float speed_deg_per_s);


