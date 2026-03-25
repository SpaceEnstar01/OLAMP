/*
 * servo_manager.cc
 * Servo module facade implementation
 *
 * Encapsulates all servo initialization, calibration, and action playback.
 * Application only needs to call Init() and PlayAction("name").
 */

#include "servo_manager.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include <string.h>

// Forward declaration for smooth multi-angle moves
static void ClampDuration(int& duration_ms) {
    const int kMinDurationMs = 100;   // avoid zero/too fast
    const int kMaxDurationMs = 30000; // 30s upper limit for safety
    if (duration_ms < kMinDurationMs) duration_ms = kMinDurationMs;
    if (duration_ms > kMaxDurationMs) duration_ms = kMaxDurationMs;
}

// ============================================
// Calibration data
// ============================================
#include "calibration/lamp0324.h"

// ============================================
// Action data - register all actions here
// ============================================
#include "actions/smile.h"
#include "actions/chat.h"
#include "actions/dance.h"
#include "actions/front_facing.h"
#include "actions/nod_head.h"
#include "actions/shaking_head.h"
#include "actions/say_hello.h"

static const char* TAG = "ServoManager";
static const char* UART_TEST_TAG = "ServoUartTest";
static constexpr UBaseType_t kServoUartTaskPriority = 7;
static constexpr int kServoUartQueueDepth = 16;
// Use the same effective UART pins as current board config (bread-compact-wifi-s3cam).
static constexpr uart_port_t SERVO_UART_NUM = UART_NUM_1;
static constexpr int SERVO_TX_PIN = 14;
static constexpr int SERVO_RX_PIN = 3;
static constexpr int SERVO_BAUD_RATE = 1000000;

// ============================================
// Servo configs built from calibration data
// ============================================
static const ServoConfig kServoConfigs[MULTI_SERVO_MAX] = {
    {1, servo_range_min[0], servo_range_max[0], servo_homing_offsets[0]},  // base_yaw
    {2, servo_range_min[1], servo_range_max[1], servo_homing_offsets[1]},  // base_pitch
    {3, servo_range_min[2], servo_range_max[2], servo_homing_offsets[2]},  // elbow_pitch
    {4, servo_range_min[3], servo_range_max[3], servo_homing_offsets[3]},  // wrist_roll
    {5, servo_range_min[4], servo_range_max[4], servo_homing_offsets[4]},  // wrist_pitch
};

// ============================================
// Action registry - add new actions here
// ============================================
struct ActionEntry {
    const char* name;
    const int16_t* data;
    int frame_count;
    int interval_ms;
};

static const ActionEntry kActionRegistry[] = {
    {"smile", smile_test03_data, SMILE_TEST03_LEN, SMILE_TEST03_INTERVAL},
    {"chat", chat_data, CHAT_LEN, CHAT_INTERVAL},
    {"dance", dance_data, DANCE_LEN, DANCE_INTERVAL},
    {"front_facing", front_facing_data, FRONT_FACING_LEN, FRONT_FACING_INTERVAL},
    {"nod_head", nod_head_data, NOD_HEAD_LEN, NOD_HEAD_INTERVAL},
    {"shaking_head", shaking_head_data, SHAKING_HEAD_LEN, SHAKING_HEAD_INTERVAL},
    {"say_hello", say_hello_data, SAY_HELLO_LEN, SAY_HELLO_INTERVAL},
};

static const int kActionCount = sizeof(kActionRegistry) / sizeof(kActionRegistry[0]);

// ============================================
// Find action by name
// ============================================
static const ActionEntry* FindAction(const char* name) {
    for (int i = 0; i < kActionCount; i++) {
        if (strcmp(kActionRegistry[i].name, name) == 0) {
            return &kActionRegistry[i];
        }
    }
    return nullptr;
}

// ============================================
// ServoManager implementation
// ============================================

ServoManager& ServoManager::GetInstance() {
    static ServoManager instance;
    return instance;
}

ServoManager::ServoManager()
    : is_playing_(false),
      last_commanded_valid_(false),
      no_feedback_mode_(true),
      servo_bus_mutex_(nullptr),
      servo_cache_mutex_(nullptr),
      servo_uart_queue_(nullptr),
      servo_uart_task_(nullptr),
      uart_seq_counter_(0),
      uart_last_done_seq_(0),
      uart_last_ok_(false) {
    for (int i = 0; i < 5; ++i) {
        last_commanded_angles_[i] = 0.0f;
        uart_last_read_angles_[i] = 0.0f;
    }
    servo_bus_mutex_ = xSemaphoreCreateMutex();
    servo_cache_mutex_ = xSemaphoreCreateMutex();
}

ServoManager::~ServoManager() {
    if (servo_uart_task_ != nullptr) {
        vTaskDelete(servo_uart_task_);
        servo_uart_task_ = nullptr;
    }
    if (servo_uart_queue_ != nullptr) {
        vQueueDelete(servo_uart_queue_);
        servo_uart_queue_ = nullptr;
    }
    if (servo_cache_mutex_ != nullptr) {
        vSemaphoreDelete(servo_cache_mutex_);
        servo_cache_mutex_ = nullptr;
    }
    if (servo_bus_mutex_ != nullptr) {
        vSemaphoreDelete(servo_bus_mutex_);
        servo_bus_mutex_ = nullptr;
    }
}

bool ServoManager::Init() {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Initializing Servo System");
    ESP_LOGI(TAG, "  UART: %d, TX: GPIO%d, RX: GPIO%d", SERVO_UART_NUM, SERVO_TX_PIN, SERVO_RX_PIN);
    ESP_LOGI(TAG, "  Baud: %d, Servos: %d", SERVO_BAUD_RATE, MULTI_SERVO_MAX);
    ESP_LOGI(TAG, "  Calibration: %s", CALIBRATION_LAMP_ID);
    ESP_LOGI(TAG, "========================================");

    if (servo_bus_mutex_ == nullptr || servo_cache_mutex_ == nullptr) {
        ESP_LOGE(TAG, "Servo mutex create failed");
        return false;
    }

    xSemaphoreTake(servo_bus_mutex_, portMAX_DELAY);

    // Step 1: Initialize UART
    if (!multi_servo_.Init(SERVO_UART_NUM, SERVO_TX_PIN, SERVO_RX_PIN, SERVO_BAUD_RATE)) {
        xSemaphoreGive(servo_bus_mutex_);
        ESP_LOGE(TAG, "UART init failed");
        return false;
    }

    // Step 2: Load calibration configs
    multi_servo_.SetAllConfigs(kServoConfigs, MULTI_SERVO_MAX);

    // Step 3: Apply calibration (write homing_offset to EEPROM)
    ESP_LOGI(TAG, "Applying calibration...");
    int ret = multi_servo_.ApplyCalibration();
    if (ret != 0) {
        ESP_LOGW(TAG, "Calibration had warnings (some servos may have failed)");
    }
    vTaskDelay(pdMS_TO_TICKS(200));

    // Step 4: Enable torque for all servos
    ESP_LOGI(TAG, "Enabling torque...");
    multi_servo_.EnableTorqueAll(true);

    if (servo_uart_queue_ == nullptr) {
        servo_uart_queue_ = xQueueCreate(kServoUartQueueDepth, sizeof(ServoUartCmd));
    }
    if (servo_uart_queue_ == nullptr) {
        xSemaphoreGive(servo_bus_mutex_);
        ESP_LOGE(TAG, "Create servo UART queue failed");
        return false;
    }
    if (servo_uart_task_ == nullptr) {
        BaseType_t t = xTaskCreatePinnedToCore(
            ServoUartTask,
            "servo_uart_comm",
            4096,
            this,
            kServoUartTaskPriority,
            &servo_uart_task_,
            1
        );
        if (t != pdPASS) {
            xSemaphoreGive(servo_bus_mutex_);
            ESP_LOGE(TAG, "Create servo UART task failed");
            return false;
        }
        ESP_LOGI(UART_TEST_TAG, "ServoUartTask started, prio=%u core=1", (unsigned)kServoUartTaskPriority);
    }

    xSemaphoreGive(servo_bus_mutex_);

    // Startup baseline from measured angles (single read), not fixed constants.
    float measured_angles[5] = {0};
    if (!TryReadAnglesOnce(measured_angles, 200)) {
        ESP_LOGE(TAG, "Init failed: unable to read startup angles for cache baseline");
        return false;
    }
    ESP_LOGI(TAG, "Startup measured baseline: [%.1f, %.1f, %.1f, %.1f, %.1f]",
             measured_angles[0], measured_angles[1], measured_angles[2],
             measured_angles[3], measured_angles[4]);

    ESP_LOGI(TAG, "Servo system ready. %d actions registered.", kActionCount);
    return true;
}

bool ServoManager::PlayAction(const char* name) {
    if (!multi_servo_.IsInitialized()) {
        ESP_LOGE(TAG, "Not initialized");
        return false;
    }

    if (is_playing_) {
        ESP_LOGW(TAG, "Action already playing, stopping first");
        Stop();
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    const ActionEntry* action = FindAction(name);
    if (!action) {
        ESP_LOGE(TAG, "Unknown action: '%s'", name);
        return false;
    }

    ESP_LOGI(TAG, "Playing action '%s' (%d frames, %dms interval)",
             action->name, action->frame_count, action->interval_ms);

    // Create playback context (allocated on heap, freed by task)
    PlaybackContext* ctx = new PlaybackContext{
        this, action->data, action->frame_count, action->interval_ms
    };

    is_playing_ = true;

    // Launch background task
    BaseType_t xRet = xTaskCreate(
        PlaybackTask,
        "servo_play",
        4096,
        ctx,
        5,       // priority
        nullptr
    );

    if (xRet != pdPASS) {
        ESP_LOGE(TAG, "Failed to create playback task");
        delete ctx;
        is_playing_ = false;
        return false;
    }

    return true;
}

void ServoManager::Stop() {
    if (is_playing_) {
        multi_servo_.StopAction();
        // Wait for task to finish
        int timeout = 50;  // 500ms max
        while (is_playing_ && timeout-- > 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

void ServoManager::PlaybackTask(void* param) {
    PlaybackContext* ctx = static_cast<PlaybackContext*>(param);
    ServoManager* mgr = ctx->manager;

    xSemaphoreTake(mgr->servo_bus_mutex_, portMAX_DELAY);
    mgr->multi_servo_.PlayAction(ctx->data, ctx->frame_count, ctx->interval_ms);
    xSemaphoreGive(mgr->servo_bus_mutex_);

    mgr->is_playing_ = false;
    delete ctx;
    vTaskDelete(nullptr);
}

bool ReadLampJointAngles(float out_angles[5]) {
    auto& mgr = ServoManager::GetInstance();
    if (!mgr.IsInitialized()) {
        ESP_LOGE(TAG, "ReadLampJointAngles: servo system not initialized");
        for (int i = 0; i < 5; ++i) {
            out_angles[i] = 0.0f;
        }
        return false;
    }
    bool ok = mgr.GetCurrentAngles(out_angles);
    if (!ok) {
        ESP_LOGW(TAG, "ReadLampJointAngles: GetCurrentAngles failed, filled with 0");
        for (int i = 0; i < 5; ++i) {
            out_angles[i] = 0.0f;
        }
        return false;
    }
    ESP_LOGI(TAG,
             "ReadLampJointAngles: base_yaw=%.2f base_pitch=%.2f elbow_pitch=%.2f "
             "wrist_roll=%.2f wrist_pitch=%.2f",
             out_angles[0], out_angles[1], out_angles[2],
             out_angles[3], out_angles[4]);
    return true;
}

bool ServoManager::GetCurrentAngles(float out[5]) {
    if (!multi_servo_.IsInitialized()) {
        ESP_LOGE(TAG, "GetCurrentAngles: servo system not initialized");
        for (int i = 0; i < 5; ++i) {
            out[i] = 0.0f;
        }
        return false;
    }
    if (servo_bus_mutex_ == nullptr) {
        ESP_LOGE(TAG, "GetCurrentAngles: servo bus mutex is null");
        return false;
    }
    if (servo_cache_mutex_ == nullptr) {
        ESP_LOGE(TAG, "GetCurrentAngles: servo cache mutex is null");
        return false;
    }

    // In no-feedback mode we report cached commanded angles as current angles.
    if (no_feedback_mode_) {
        xSemaphoreTake(servo_cache_mutex_, portMAX_DELAY);
        if (!last_commanded_valid_) {
            xSemaphoreGive(servo_cache_mutex_);
            ESP_LOGW(TAG, "GetCurrentAngles(no-feedback): cache unavailable");
            for (int i = 0; i < 5; ++i) out[i] = 0.0f;
            return false;
        }
        for (int i = 0; i < 5; ++i) {
            out[i] = last_commanded_angles_[i];
        }
        xSemaphoreGive(servo_cache_mutex_);
        return true;
    }
    return TryReadAnglesOnce(out, 50);
}

bool ServoManager::GetAnglesForMotion(float out[5]) {
    if (!multi_servo_.IsInitialized()) {
        ESP_LOGE(TAG, "GetAnglesForMotion: servo system not initialized");
        return false;
    }
    if (servo_cache_mutex_ == nullptr) {
        ESP_LOGE(TAG, "GetAnglesForMotion: servo cache mutex is null");
        return false;
    }

    xSemaphoreTake(servo_cache_mutex_, portMAX_DELAY);
    if (last_commanded_valid_) {
        for (int i = 0; i < 5; ++i) {
            out[i] = last_commanded_angles_[i];
        }
        xSemaphoreGive(servo_cache_mutex_);
        return true;
    }
    xSemaphoreGive(servo_cache_mutex_);

    if (no_feedback_mode_) {
        ESP_LOGW(TAG, "GetAnglesForMotion(no-feedback): cache unavailable");
        return false;
    }

    if (TryReadAnglesOnce(out, 50)) {
        return true;
    }

    ESP_LOGW(TAG, "GetAnglesForMotion: no cached angles and one-shot read failed");
    return false;
}

void ServoManager::UpdateLastCommandedAngles(const float angles[5]) {
    if (angles == nullptr || servo_cache_mutex_ == nullptr) {
        return;
    }
    xSemaphoreTake(servo_cache_mutex_, portMAX_DELAY);
    for (int i = 0; i < 5; ++i) {
        last_commanded_angles_[i] = angles[i];
    }
    last_commanded_valid_ = true;
    xSemaphoreGive(servo_cache_mutex_);
}

void ServoManager::SetInitialAngles(const float angles[5]) {
    if (angles == nullptr || servo_cache_mutex_ == nullptr) {
        return;
    }
    xSemaphoreTake(servo_cache_mutex_, portMAX_DELAY);
    for (int i = 0; i < 5; ++i) {
        last_commanded_angles_[i] = angles[i];
    }
    last_commanded_valid_ = true;
    xSemaphoreGive(servo_cache_mutex_);
}

bool ServoManager::MoveToAngles(const float angles[5], int duration_ms) {
    if (!multi_servo_.IsInitialized()) {
        ESP_LOGE(TAG, "MoveToAngles: servo system not initialized");
        return false;
    }

    ClampDuration(duration_ms);

    // Never start from a fixed value; use cached/real motion baseline only.
    float start[5] = {0};
    if (!GetAnglesForMotion(start)) {
        ESP_LOGE(TAG, "MoveToAngles: no valid start angles (cache/read unavailable), reject motion");
        return false;
    }
    ESP_LOGI(TAG, "MoveToAngles start: base_yaw=%.1f base_pitch=%.1f elbow_pitch=%.1f wrist_roll=%.1f wrist_pitch=%.1f",
             start[0], start[1], start[2], start[3], start[4]);
    ESP_LOGI(TAG, "MoveToAngles target: base_yaw=%.1f base_pitch=%.1f elbow_pitch=%.1f wrist_roll=%.1f wrist_pitch=%.1f duration_ms=%d",
             angles[0], angles[1], angles[2], angles[3], angles[4], duration_ms);

    if (servo_bus_mutex_ == nullptr) {
        ESP_LOGE(TAG, "MoveToAngles: servo bus mutex is null");
        return false;
    }
    xSemaphoreTake(servo_bus_mutex_, portMAX_DELAY);
    bool torque_ok = multi_servo_.EnableTorqueAll(true);
    ESP_LOGI(TAG, "MoveToAngles: protective EnableTorqueAll(true) => %d", torque_ok ? 1 : 0);

    bool all_ok = true;
    // Finer step interval (20ms) for smoother motion; smoothstep for natural ease-in-out.
    const int kStepMs = 20;
    int steps = duration_ms / kStepMs;
    if (steps < 1) steps = 1;

    for (int s = 1; s <= steps; ++s) {
        float t_linear = static_cast<float>(s) / static_cast<float>(steps);
        float t = t_linear * t_linear * (3.0f - 2.0f * t_linear);  // smoothstep
        float step_angles[5];
        for (int i = 0; i < 5; ++i) {
            step_angles[i] = start[i] + (angles[i] - start[i]) * t;
        }
        if (!multi_servo_.SyncMoveTo(step_angles, 5, 0, 0)) {
            all_ok = false;
        }
        vTaskDelay(pdMS_TO_TICKS(kStepMs));
    }
    xSemaphoreGive(servo_bus_mutex_);

    if (all_ok) {
        UpdateLastCommandedAngles(angles);
    } else {
        ESP_LOGW(TAG, "MoveToAngles: one or more SyncMoveTo writes failed, cache not updated");
    }

    return all_ok;
}

bool ServoManager::MoveJointToAngle(uint8_t joint_index, float target_angle_deg, int duration_ms) {
    if (!multi_servo_.IsInitialized()) {
        ESP_LOGE(TAG, "MoveJointToAngle: servo system not initialized");
        return false;
    }
    if (joint_index >= 5) {
        ESP_LOGE(TAG, "MoveJointToAngle: invalid joint_index=%u", joint_index);
        return false;
    }
    if (servo_uart_queue_ == nullptr || servo_cache_mutex_ == nullptr) {
        ESP_LOGE(TAG, "MoveJointToAngle: servo UART infrastructure unavailable");
        return false;
    }

    // Use cached baseline only in no-feedback mode.
    float current[5] = {0};
    if (!GetAnglesForMotion(current)) {
        ESP_LOGE(TAG, "MoveJointToAngle: no motion baseline");
        return false;
    }

    float current_deg = current[joint_index];
    float delta_abs = target_angle_deg - current_deg;
    if (delta_abs < 0.0f) delta_abs = -delta_abs;
    if (delta_abs < 1e-3f) {
        return true;
    }

    // Derive a conservative speed register from requested duration.
    // Keep within practical range to avoid too fast/too slow behavior.
    if (duration_ms < 300) duration_ms = 300;
    if (duration_ms > 3000) duration_ms = 3000;
    int speed_reg = static_cast<int>(1200.0f * (delta_abs / static_cast<float>(duration_ms)));
    if (speed_reg < 80) speed_reg = 80;
    if (speed_reg > 800) speed_reg = 800;

    ServoUartCmd cmd{};
    cmd.type = ServoUartCmdType::kWriteJoint;
    cmd.joint_index = joint_index;
    cmd.target_angle_deg = target_angle_deg;
    cmd.speed_reg = speed_reg;
    bool ok = SubmitUartCmdAndWait(cmd, 400);

    ESP_LOGI(TAG,
             "MoveJointToAngle: joint=%u current=%.1f target=%.1f duration_ms=%d speed_reg=%d ok=%d",
             joint_index + 1, current_deg, target_angle_deg, duration_ms, speed_reg, ok ? 1 : 0);

    if (!ok) {
        return false;
    }
    return true;
}

bool ServoManager::TryReadAnglesOnce(float out[5], int timeout_ms) {
    if (!multi_servo_.IsInitialized() || servo_uart_queue_ == nullptr || servo_cache_mutex_ == nullptr) {
        return false;
    }
    if (timeout_ms < 10) timeout_ms = 10;

    ServoUartCmd cmd{};
    cmd.type = ServoUartCmdType::kReadOnce;
    bool ok = SubmitUartCmdAndWait(cmd, timeout_ms);
    if (!ok) {
        return false;
    }

    xSemaphoreTake(servo_cache_mutex_, portMAX_DELAY);
    for (int i = 0; i < 5; ++i) {
        out[i] = uart_last_read_angles_[i];
    }
    xSemaphoreGive(servo_cache_mutex_);
    return true;
}

bool ServoManager::SubmitUartCmdAndWait(ServoUartCmd& cmd, int timeout_ms) {
    if (servo_uart_queue_ == nullptr || servo_cache_mutex_ == nullptr) {
        return false;
    }
    if (timeout_ms < 10) timeout_ms = 10;

    xSemaphoreTake(servo_cache_mutex_, portMAX_DELAY);
    cmd.requester_task = xTaskGetCurrentTaskHandle();
    cmd.seq_id = ++uart_seq_counter_;
    xSemaphoreGive(servo_cache_mutex_);

    BaseType_t enq = xQueueSend(servo_uart_queue_, &cmd, pdMS_TO_TICKS(20));
    if (enq != pdTRUE) {
        ESP_LOGW(UART_TEST_TAG, "queue full, drop cmd type=%d", static_cast<int>(cmd.type));
        return false;
    }

    uint32_t n = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(timeout_ms));
    if (n == 0) {
        ESP_LOGW(UART_TEST_TAG, "wait timeout type=%d seq=%u", static_cast<int>(cmd.type), (unsigned)cmd.seq_id);
        return false;
    }

    xSemaphoreTake(servo_cache_mutex_, portMAX_DELAY);
    bool seq_match = (uart_last_done_seq_ == cmd.seq_id);
    bool ok = uart_last_ok_;
    xSemaphoreGive(servo_cache_mutex_);
    return seq_match && ok;
}

void ServoManager::ServoUartTask(void* param) {
    auto* mgr = static_cast<ServoManager*>(param);
    ServoUartCmd cmd{};
    for (;;) {
        if (xQueueReceive(mgr->servo_uart_queue_, &cmd, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        bool ok = false;
        if (cmd.type == ServoUartCmdType::kWriteJoint) {
            xSemaphoreTake(mgr->servo_bus_mutex_, portMAX_DELAY);
            bool torque_ok = mgr->multi_servo_.EnableTorqueAll(true);
            bool sent_ok = mgr->multi_servo_.MoveTo(cmd.joint_index, cmd.target_angle_deg, cmd.speed_reg, 0);
            xSemaphoreGive(mgr->servo_bus_mutex_);
            if (!torque_ok) {
                ESP_LOGW(UART_TEST_TAG, "UART_CMD write: torque enable not acknowledged, continue");
            }
            // Write command accepted/sent is enough for motion success in this mode.
            ok = sent_ok;

            if (ok && mgr->servo_cache_mutex_ != nullptr) {
                xSemaphoreTake(mgr->servo_cache_mutex_, portMAX_DELAY);
                if (!mgr->last_commanded_valid_) {
                    for (int i = 0; i < 5; ++i) mgr->last_commanded_angles_[i] = 0.0f;
                }
                mgr->last_commanded_angles_[cmd.joint_index] = cmd.target_angle_deg;
                mgr->last_commanded_valid_ = true;
                ESP_LOGI(UART_TEST_TAG, "Cached angles: [%.1f, %.1f, %.1f, %.1f, %.1f]",
                         mgr->last_commanded_angles_[0], mgr->last_commanded_angles_[1],
                         mgr->last_commanded_angles_[2], mgr->last_commanded_angles_[3],
                         mgr->last_commanded_angles_[4]);
                xSemaphoreGive(mgr->servo_cache_mutex_);
            }
            ESP_LOGI(UART_TEST_TAG,
                     "UART_CMD write joint=%u target=%.1f speed=%d seq=%u ok=%d",
                     cmd.joint_index + 1, cmd.target_angle_deg, cmd.speed_reg,
                     (unsigned)cmd.seq_id, ok ? 1 : 0);
        } else if (cmd.type == ServoUartCmdType::kReadOnce) {
            float read_buf[5] = {0};
            xSemaphoreTake(mgr->servo_bus_mutex_, portMAX_DELAY);
            ok = mgr->multi_servo_.ReadCurrentAngles(read_buf);
            xSemaphoreGive(mgr->servo_bus_mutex_);

            if (ok && mgr->servo_cache_mutex_ != nullptr) {
                xSemaphoreTake(mgr->servo_cache_mutex_, portMAX_DELAY);
                for (int i = 0; i < 5; ++i) {
                    mgr->uart_last_read_angles_[i] = read_buf[i];
                    mgr->last_commanded_angles_[i] = read_buf[i];
                }
                mgr->last_commanded_valid_ = true;
                xSemaphoreGive(mgr->servo_cache_mutex_);
            }
            ESP_LOGI(UART_TEST_TAG, "UART_CMD read_once seq=%u ok=%d", (unsigned)cmd.seq_id, ok ? 1 : 0);
        }

        if (mgr->servo_cache_mutex_ != nullptr) {
            xSemaphoreTake(mgr->servo_cache_mutex_, portMAX_DELAY);
            mgr->uart_last_done_seq_ = cmd.seq_id;
            mgr->uart_last_ok_ = ok;
            xSemaphoreGive(mgr->servo_cache_mutex_);
        }
        if (cmd.requester_task != nullptr) {
            xTaskNotifyGive(cmd.requester_task);
        }
    }
}


