
#include "lamp_mcp_bridge.h"

#include "esp_log.h"

#include "lamp_mapping.h"
#include "servo/servo_manager.h"

static const char* TAG = "LampMcpBridge";

// Conservative limits for joint 1/2/5.
static constexpr float BASE_YAW_MIN_DEG    = -90.0f;
static constexpr float BASE_YAW_MAX_DEG    =  90.0f;
static constexpr float BASE_PITCH_MIN_DEG  = -10.0f;   // joint2
static constexpr float BASE_PITCH_MAX_DEG  =  110.0f;  // joint2
static constexpr float WRIST_PITCH_MIN_DEG = -90.0f;
static constexpr float WRIST_PITCH_MAX_DEG =  110.0f;

bool LampSendPreset(const char* name, int duration_ms) {
    if (!name) {
        ESP_LOGE(TAG, "LampSendPreset: name is null");
        return false;
    }

    LampCommand cmd{};
    cmd.type = LampActionType::GO_PRESET;
    cmd.preset_name = name;
    cmd.duration_ms = duration_ms;

    if (!ResolvePresetCommand(cmd)) {
        ESP_LOGE(TAG, "LampSendPreset: preset '%s' not found", name);
        return false;
    }

    return ServoManager::GetInstance().MoveToAngles(cmd.angles, cmd.duration_ms);
}

bool LampSendAngles(const float angles[5], int duration_ms) {
    if (!angles) {
        ESP_LOGE(TAG, "LampSendAngles: angles is null");
        return false;
    }

    float local_angles[5];
    for (int i = 0; i < 5; ++i) {
        local_angles[i] = angles[i];
    }

    return ServoManager::GetInstance().MoveToAngles(local_angles, duration_ms);
}

bool LampMoveJoint1Relative(float delta_deg, float speed_deg_per_s) {
    auto& mgr = ServoManager::GetInstance();
    if (!mgr.IsInitialized()) {
        ESP_LOGE(TAG, "LampMoveJoint1Relative: servo system not initialized");
        return false;
    }

    float current[5] = {0};
    if (!mgr.GetAnglesForMotion(current)) {
        ESP_LOGE(TAG, "LampMoveJoint1Relative: no valid motion baseline (cache/read unavailable)");
        return false;
    }
    float current_1 = current[0];
    float target_1  = current_1 + delta_deg;

    if (target_1 < BASE_YAW_MIN_DEG) target_1 = BASE_YAW_MIN_DEG;
    if (target_1 > BASE_YAW_MAX_DEG) target_1 = BASE_YAW_MAX_DEG;

    float delta_abs = target_1 - current_1;
    if (delta_abs < 0.0f) delta_abs = -delta_abs;
    if (delta_abs < 1e-3f) {
        ESP_LOGI(TAG, "LampMoveJoint1Relative: delta too small, skip (current_1=%.2f)", current_1);
        return true;
    }

    if (speed_deg_per_s <= 0.0f) {
        speed_deg_per_s = 15.0f;
    }
    float duration_s = delta_abs / speed_deg_per_s;
    int duration_ms = static_cast<int>(duration_s * 1000.0f);
    if (duration_ms < 300)  duration_ms = 300;
    if (duration_ms > 3000) duration_ms = 3000;

    ESP_LOGI(TAG,
             "LampMoveJoint1Relative: current_1=%.1f target_1=%.1f delta=%.1f duration_ms=%d",
             current_1, target_1, target_1 - current_1, duration_ms);

    // Single-joint lightweight path: avoid 5-joint interpolation flood.
    return mgr.MoveJointToAngle(0, target_1, duration_ms);
}

bool LampMoveJoint2Relative(float delta_deg, float speed_deg_per_s) {
    auto& mgr = ServoManager::GetInstance();
    if (!mgr.IsInitialized()) {
        ESP_LOGE(TAG, "LampMoveJoint2Relative: servo system not initialized");
        return false;
    }

    float current[5] = {0};
    if (!mgr.GetAnglesForMotion(current)) {
        ESP_LOGE(TAG, "LampMoveJoint2Relative: no valid motion baseline (cache/read unavailable)");
        return false;
    }
    float current_2 = current[1];
    // Joint2 mechanical direction is opposite to command semantic:
    // semantic +delta(up) => physical angle decrease.
    float target_2 = current_2 - delta_deg;

    if (target_2 < BASE_PITCH_MIN_DEG) target_2 = BASE_PITCH_MIN_DEG;
    if (target_2 > BASE_PITCH_MAX_DEG) target_2 = BASE_PITCH_MAX_DEG;

    float delta_abs = target_2 - current_2;
    if (delta_abs < 0.0f) delta_abs = -delta_abs;
    if (delta_abs < 1e-3f) {
        ESP_LOGI(TAG, "LampMoveJoint2Relative: delta too small, skip (current_2=%.2f)", current_2);
        return true;
    }

    if (speed_deg_per_s <= 0.0f) {
        speed_deg_per_s = 12.0f;
    }
    float duration_s = delta_abs / speed_deg_per_s;
    int duration_ms = static_cast<int>(duration_s * 1000.0f);
    if (duration_ms < 300)  duration_ms = 300;
    if (duration_ms > 3000) duration_ms = 3000;

    ESP_LOGI(TAG,
             "LampMoveJoint2Relative: current_2=%.1f target_2=%.1f delta=%.1f duration_ms=%d",
             current_2, target_2, target_2 - current_2, duration_ms);

    return mgr.MoveJointToAngle(1, target_2, duration_ms);
}

bool LampMoveJoint5Relative(float delta_deg, float speed_deg_per_s) {
    auto& mgr = ServoManager::GetInstance();
    if (!mgr.IsInitialized()) {
        ESP_LOGE(TAG, "LampMoveJoint5Relative: servo system not initialized");
        return false;
    }

    float current[5] = {0};
    if (!mgr.GetAnglesForMotion(current)) {
        ESP_LOGE(TAG, "LampMoveJoint5Relative: no valid motion baseline (cache/read unavailable)");
        return false;
    }
    float current_5 = current[4];
    float target_5 = current_5 + delta_deg;

    if (target_5 < WRIST_PITCH_MIN_DEG) target_5 = WRIST_PITCH_MIN_DEG;
    if (target_5 > WRIST_PITCH_MAX_DEG) target_5 = WRIST_PITCH_MAX_DEG;

    float delta_abs = target_5 - current_5;
    if (delta_abs < 0.0f) delta_abs = -delta_abs;
    if (delta_abs < 1e-3f) {
        ESP_LOGI(TAG, "LampMoveJoint5Relative: delta too small, skip (current_5=%.2f)", current_5);
        return true;
    }

    if (speed_deg_per_s <= 0.0f) {
        speed_deg_per_s = 10.0f;
    }
    float duration_s = delta_abs / speed_deg_per_s;
    int duration_ms = static_cast<int>(duration_s * 1000.0f);
    if (duration_ms < 300)  duration_ms = 300;
    if (duration_ms > 3000) duration_ms = 3000;

    ESP_LOGI(TAG,
             "LampMoveJoint5Relative: current_5=%.1f target_5=%.1f delta=%.1f duration_ms=%d",
             current_5, target_5, target_5 - current_5, duration_ms);

    // Single-joint lightweight path: avoid 5-joint interpolation flood.
    return mgr.MoveJointToAngle(4, target_5, duration_ms);
}


