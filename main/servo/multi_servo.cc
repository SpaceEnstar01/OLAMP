/*
 * multi_servo.cc
 * Multi-servo controller implementation with lerobot-compatible angle conversion
 */

#include "multi_servo.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"
#include <string.h>

static const char *TAG = "MultiServo";

MultiServo::MultiServo()
    : servo_count_(0),
      initialized_(false),
      stop_requested_(false) {
    memset(configs_, 0, sizeof(configs_));
}

MultiServo::~MultiServo() {
}

bool MultiServo::Init(uart_port_t uart_num, int tx_pin, int rx_pin, int baud_rate) {
    if (initialized_) {
        ESP_LOGW(TAG, "Already initialized");
        return true;
    }

    ESP_LOGI(TAG, "Initializing UART: UART_NUM_%d, TX=%d, RX=%d, Baud=%d",
             uart_num, tx_pin, rx_pin, baud_rate);

    int ret = servo_.init(uart_num, tx_pin, rx_pin, baud_rate);
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to initialize UART! Error: %d", ret);
        return false;
    }

    vTaskDelay(pdMS_TO_TICKS(100));
    initialized_ = true;
    ESP_LOGI(TAG, "Initialized successfully");
    return true;
}

void MultiServo::SetServoConfig(uint8_t index, const ServoConfig& config) {
    if (index >= MULTI_SERVO_MAX) {
        ESP_LOGE(TAG, "Invalid servo index: %d", index);
        return;
    }
    configs_[index] = config;
    if (index >= servo_count_) {
        servo_count_ = index + 1;
    }
}

void MultiServo::SetAllConfigs(const ServoConfig configs[], int count) {
    if (count > MULTI_SERVO_MAX) {
        ESP_LOGW(TAG, "Clamping servo count from %d to %d", count, MULTI_SERVO_MAX);
        count = MULTI_SERVO_MAX;
    }
    for (int i = 0; i < count; i++) {
        configs_[i] = configs[i];
    }
    servo_count_ = count;
}

int MultiServo::ApplyCalibration() {
    if (!initialized_) {
        ESP_LOGE(TAG, "Not initialized");
        return -1;
    }

    ESP_LOGI(TAG, "Applying calibration to %d servos", servo_count_);
    int success = 0, fail = 0;

    for (int i = 0; i < servo_count_; i++) {
        uint8_t id = configs_[i].id;
        int16_t offset = configs_[i].homing_offset;

        // Unlock EEPROM
        int ret = servo_.unLockEprom(id);
        if (ret != 1) {
            ESP_LOGW(TAG, "Failed to unlock EEPROM for ID %d", id);
            fail++;
            continue;
        }
        vTaskDelay(pdMS_TO_TICKS(50));

        // Write homing offset
        uint16_t offset_u16 = (uint16_t)((int16_t)offset);
        ret = servo_.writeWord(id, 31, offset_u16);
        if (ret != 1) {
            ESP_LOGW(TAG, "Failed to write offset for ID %d", id);
            servo_.LockEprom(id);
            fail++;
            continue;
        }
        vTaskDelay(pdMS_TO_TICKS(50));

        // Lock EEPROM
        servo_.LockEprom(id);
        vTaskDelay(pdMS_TO_TICKS(50));

        ESP_LOGI(TAG, "Servo ID %d calibrated (offset: %d)", id, offset);
        success++;
    }

    ESP_LOGI(TAG, "Calibration done: %d success, %d fail", success, fail);
    return (fail > 0) ? -1 : 0;
}

int16_t MultiServo::AngleToPosition(float angle, uint8_t index) {
    if (index >= servo_count_) {
        ESP_LOGE(TAG, "Invalid servo index: %d", index);
        return 0;
    }

    // Lerobot-compatible conversion:
    // mid = (range_min + range_max) / 2
    // position = (angle * 4095 / 360) + mid
    float mid = ((float)configs_[index].range_min +
                 (float)configs_[index].range_max) / 2.0f;

    float position = (angle * 4095.0f / 360.0f) + mid;

    // Clamp to valid range
    if (position < (float)configs_[index].range_min) {
        position = (float)configs_[index].range_min;
    }
    if (position > (float)configs_[index].range_max) {
        position = (float)configs_[index].range_max;
    }

    return (int16_t)position;
}

float MultiServo::PositionToAngle(int16_t position, uint8_t index) {
    if (index >= servo_count_) {
        return 0.0f;
    }

    float mid = ((float)configs_[index].range_min +
                 (float)configs_[index].range_max) / 2.0f;

    return ((float)position - mid) * 360.0f / 4095.0f;
}

bool MultiServo::MoveTo(uint8_t index, float angle, int speed, int acc) {
    if (!initialized_ || index >= servo_count_) {
        return false;
    }

    int16_t pos = AngleToPosition(angle, index);
    int ret = servo_.WritePosEx(configs_[index].id, pos, speed, acc);
    // In no-feedback mode, ret>=0 means command was accepted/sent.
    return (ret >= 0);
}

bool MultiServo::SyncMoveTo(const float angles[], int count, int speed, int acc) {
    if (!initialized_) return false;
    if (count > servo_count_) count = servo_count_;

    bool all_ok = true;
    for (int i = 0; i < count; i++) {
        int16_t pos = AngleToPosition(angles[i], i);
        uint8_t id = configs_[i].id;
        int ret = servo_.WritePosEx(id, pos, speed, acc);
        ESP_LOGI(TAG, "SyncMoveTo: idx=%d id=%d pos=%d speed=%d acc=%d ret=%d",
                 i, id, pos, speed, acc, ret);
        if (ret < 0) all_ok = false;
    }
    return all_ok;
}

bool MultiServo::ReadCurrentAngles(float out[5]) {
    if (!initialized_ || servo_count_ == 0) {
        ESP_LOGE(TAG, "ReadCurrentAngles: not initialized or servo_count_==0 (init=%d, count=%d)",
                 initialized_, servo_count_);
        for (int i = 0; i < 5; ++i) {
            out[i] = 0.0f;
        }
        return false;
    }

    bool all_ok = true;
    for (int i = 0; i < 5; ++i) {
        if (i >= servo_count_) {
            ESP_LOGW(TAG, "ReadCurrentAngles: index %d >= servo_count_ %d, fill 0", i, servo_count_);
            out[i] = 0.0f;
            continue;
        }

        uint8_t id = configs_[i].id;
        int pos = servo_.ReadPos(id);
        int err = servo_.getErr();

        if (pos < 0 || err != 0) {
            ESP_LOGW(TAG, "ReadCurrentAngles: idx=%d id=%d ReadPos failed, pos=%d err=%d",
                     i, id, pos, err);
            out[i] = 0.0f;
            all_ok = false;
        } else {
            float angle = PositionToAngle(static_cast<int16_t>(pos), i);
            out[i] = angle;
            ESP_LOGI(TAG, "ReadCurrentAngles: idx=%d id=%d pos=%d angle=%.2f deg",
                     i, id, pos, angle);
        }
        // Small delay between servo reads to let the half-duplex bus settle.
        esp_rom_delay_us(2000);
    }
    return all_ok;
}

bool MultiServo::PlayAction(const int16_t data[], int frame_count, int interval_ms) {
    if (!initialized_ || servo_count_ == 0) {
        ESP_LOGE(TAG, "Not ready to play action");
        return false;
    }

    stop_requested_ = false;
    ESP_LOGI(TAG, "Playing action: %d frames, %dms interval", frame_count, interval_ms);

    for (int f = 0; f < frame_count; f++) {
        if (stop_requested_) {
            ESP_LOGI(TAG, "Action stopped at frame %d", f);
            return false;
        }

        // Each frame has MULTI_SERVO_MAX angle values
        const int16_t* frame = &data[f * MULTI_SERVO_MAX];
        for (int s = 0; s < servo_count_; s++) {
            float angle = (float)frame[s];
            int16_t pos = AngleToPosition(angle, s);
            servo_.WritePosEx(configs_[s].id, pos, 0, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(interval_ms));
    }

    ESP_LOGI(TAG, "Action playback completed");
    return true;
}

void MultiServo::StopAction() {
    stop_requested_ = true;
}

bool MultiServo::EnableTorqueAll(bool enable) {
    if (!initialized_) return false;

    bool all_ok = true;
    for (int i = 0; i < servo_count_; i++) {
        int ret = servo_.EnableTorque(configs_[i].id, enable ? 1 : 0);
        if (ret < 0) all_ok = false;
    }
    return all_ok;
}

















