#include "servo.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/uart.h"  // 在文件开头添加
#define TAG "Servo"

Servo::Servo() 
    : initialized_(false), 
      servo_id_(1), 
      error_code_(0) {
}

Servo::~Servo() {
}

bool Servo::Init(uart_port_t uart_num, int tx_pin, int rx_pin, int baud_rate, int servo_id) {
    if (initialized_) {
        ESP_LOGW(TAG, "Servo already initialized");
        return true;
    }

    servo_id_ = servo_id;
    error_code_ = 0;

    ESP_LOGI(TAG, "Initializing servo UART: UART_NUM_%d, TX=%d, RX=%d, Baud=%d, ID=%d", 
             uart_num, tx_pin, rx_pin, baud_rate, servo_id_);

    // 初始化UART
    int ret = servo_.init(uart_num, tx_pin, rx_pin, baud_rate);
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to initialize UART! Error: %d", ret);
        error_code_ = ret;
        return false;
    }

    ESP_LOGI(TAG, "UART initialized successfully");

    // 等待UART稳定
    vTaskDelay(pdMS_TO_TICKS(100));

    // 尝试Ping舵机，检查通信是否正常
    ESP_LOGI(TAG, "Pinging servo ID %d...", servo_id_);
    int ping_result = servo_.Ping(servo_id_);
    if (ping_result == servo_id_) {
        ESP_LOGI(TAG, "Servo ID %d responded successfully!", servo_id_);
    } else if (ping_result == -1) {
        ESP_LOGW(TAG, "Servo ID %d did not respond. Please check:", servo_id_);
        ESP_LOGW(TAG, "  1. Power supply to servo");
        ESP_LOGW(TAG, "  2. TX/RX connections");
        ESP_LOGW(TAG, "  3. GND connection");
        ESP_LOGW(TAG, "  4. Servo ID setting");
        ESP_LOGW(TAG, "Continuing anyway...");
    } else {
        ESP_LOGW(TAG, "Unexpected ping result: %d", ping_result);
    }

    // 使能舵机力矩
    ESP_LOGI(TAG, "Enabling torque for servo ID %d...", servo_id_);
    ret = servo_.EnableTorque(servo_id_, 1);
    if (ret == 1) {
        ESP_LOGI(TAG, "Torque enabled successfully");
    } else {
        ESP_LOGW(TAG, "Failed to enable torque, error: %d (no response from servo)", ret);
    }

    initialized_ = true;
    ESP_LOGI(TAG, "Servo initialization completed");
    return true;
}

bool Servo::MoveTo(float degree, int speed, int acc) {
    if (!initialized_) {
        ESP_LOGE(TAG, "Servo not initialized, cannot move");
        error_code_ = -1;
        return false;
    }

    // 限制角度范围
    if (degree < -180.0f) degree = -180.0f;
    if (degree > 180.0f) degree = 180.0f;

    int target_pos_reg = DegreeToReg(degree);
    ESP_LOGI(TAG, "Moving servo ID %d to %.1f degrees (reg value: %d, Speed: %d, ACC: %d)", 
             servo_id_, degree, target_pos_reg, speed, acc);

    int ret = servo_.WritePosEx(servo_id_, target_pos_reg, speed, acc);
    // 注意：Ack返回1表示成功，0表示失败（与通常的返回值逻辑相反）
    if (ret == 1) {
        ESP_LOGI(TAG, "Command sent successfully");
        error_code_ = 0;
        return true;
    } else {
        ESP_LOGW(TAG, "Command failed, error: %d", ret);
        error_code_ = ret;
        return false;
    }
}

float Servo::GetPosition() {
    if (!initialized_) {
        ESP_LOGE(TAG, "Servo not initialized, cannot read position");
        error_code_ = -1;
        return 0.0f / 0.0f; // 返回 NaN
    }

    int pos_reg = servo_.ReadPos(servo_id_);
    if (servo_.getErr() == 0) {
        float pos_degree = RegToDegree(pos_reg);
        error_code_ = 0;
        return pos_degree;
    } else {
        ESP_LOGW(TAG, "Failed to read position, error: %d", servo_.getErr());
        error_code_ = servo_.getErr();
        return 0.0f / 0.0f; // 返回 NaN
    }
}

bool Servo::EnableTorque(bool enable) {
    if (!initialized_) {
        ESP_LOGE(TAG, "Servo not initialized, cannot enable/disable torque");
        error_code_ = -1;
        return false;
    }

    int ret = servo_.EnableTorque(servo_id_, enable ? 1 : 0);
    if (ret == 1) {
        ESP_LOGI(TAG, "Torque %s successfully", enable ? "enabled" : "disabled");
        error_code_ = 0;
        return true;
    } else {
        ESP_LOGW(TAG, "Failed to %s torque, error: %d", enable ? "enable" : "disable", ret);
        error_code_ = ret;
        return false;
    }
}

int Servo::Ping() {
    if (!initialized_) {
        ESP_LOGE(TAG, "Servo not initialized, cannot ping");
        error_code_ = -1;
        return -1;
    }

    int ping_result = servo_.Ping(servo_id_);
    if (ping_result == servo_id_) {
        error_code_ = 0;
    } else {
        error_code_ = -1;
    }
    return ping_result;
}

int Servo::DegreeToReg(float degree) {
    return (int)(degree / 0.087f);
}

float Servo::RegToDegree(int reg) {
    return reg * 0.087f;
}