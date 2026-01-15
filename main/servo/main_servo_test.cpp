#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "st_servo/SMS_STS.h"

static const char *TAG = "ST_SERVO";

// ============================================
// UART 引脚配置（根据你的连接修改）
// ============================================
// 微雪驱动板连接说明（重要！）：
// 根据微雪文档：跳线帽在A位置时，连接方式必须是 RX-RX、TX-TX 直连！
// 微雪驱动板 TX → ESP32-S3 GPIO17 (TX) - 直连
// 微雪驱动板 RX → ESP32-S3 GPIO18 (RX) - 直连
// 微雪驱动板 GND → ESP32-S3 GND
// 注意：微雪驱动板内部已做交叉，所以外部需要直连
#define SERVO_UART_NUM      UART_NUM_1
#define SERVO_TX_PIN        17      // ESP32-S3 发送引脚（直连到微雪TX）
#define SERVO_RX_PIN        18      // ESP32-S3 接收引脚（直连到微雪RX）
// 波特率配置（STS舵机默认波特率为1M，必须使用1M才能通信！）
#define SERVO_BAUD_RATE     1000000  // STS舵机默认波特率：1Mbps（必须使用此波特率）
// 注意：STS舵机出厂默认是1M，如果改为其他波特率，需要先用地面站软件配置舵机

// ============================================
// 舵机配置
// ============================================
// 舵机ID配置（STS3215默认ID是1）
#define SERVO_ID            1       // STS3215舵机ID

// 使用 extern "C" 确保 app_main 函数可以被 C 代码调用（ESP-IDF启动代码是C代码）
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "ST Servo ESP32-S3 Project");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "UART Configuration:");
    ESP_LOGI(TAG, "  UART Number: UART_NUM_%d", SERVO_UART_NUM);
    ESP_LOGI(TAG, "  TX Pin: GPIO%d (ESP32-S3发送 -> 微雪驱动板RX)", SERVO_TX_PIN);
    ESP_LOGI(TAG, "  RX Pin: GPIO%d (ESP32-S3接收 <- 微雪驱动板TX)", SERVO_RX_PIN);
    ESP_LOGI(TAG, "  Baud Rate: %d", SERVO_BAUD_RATE);
    ESP_LOGI(TAG, "Servo ID: %d", SERVO_ID);
    ESP_LOGI(TAG, "========================================");
    
    // 初始化 SMS_STS 舵机
    SMS_STS servo;
    int ret = servo.init(SERVO_UART_NUM, SERVO_TX_PIN, SERVO_RX_PIN, SERVO_BAUD_RATE);
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to initialize UART! Error: %d", ret);
        return;
    }
    ESP_LOGI(TAG, "UART initialized successfully");
    
    // 等待UART稳定
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // 尝试Ping舵机，检查通信是否正常
    ESP_LOGI(TAG, "Pinging servo ID %d...", SERVO_ID);
    int ping_result = servo.Ping(SERVO_ID);
    if (ping_result == SERVO_ID) {
        ESP_LOGI(TAG, "Servo ID %d responded successfully!", SERVO_ID);
    } else if (ping_result == -1) {
        ESP_LOGW(TAG, "Servo ID %d did not respond. Please check:", SERVO_ID);
        ESP_LOGW(TAG, "  1. Power supply to servo");
        ESP_LOGW(TAG, "  2. TX/RX connections");
        ESP_LOGW(TAG, "  3. GND connection");
        ESP_LOGW(TAG, "  4. Servo ID setting");
        ESP_LOGW(TAG, "Continuing anyway...");
    } else {
        ESP_LOGW(TAG, "Unexpected ping result: %d", ping_result);
    }
    
    // 使能舵机力矩
    ESP_LOGI(TAG, "Enabling torque for servo ID %d...", SERVO_ID);
    ret = servo.EnableTorque(SERVO_ID, 1);
    if (ret == 1) {  // 修复：Ack返回1表示成功，0表示失败
        ESP_LOGI(TAG, "Torque enabled successfully");
    } else {
        ESP_LOGW(TAG, "Failed to enable torque, error: %d (no response from servo)", ret);
    }
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Starting control loop...");
    ESP_LOGI(TAG, "========================================");
    
    // 舵机运动参数配置（根据寄存器说明修正）
    // 重要：根据寄存器文档，参数逻辑是反向的！
    // ACC (地址0x29): 0 = 最大加速度，值越大加速度越小 (范围: 0-254)
    // Speed (地址0x2E): 0 = 最大速度，值越大速度越小 (范围: 0-32767)
    // 注意：速度过快可能导致舵机抖动，需要根据实际负载调整
    #define SERVO_SPEED      0       // 速度：0 = 最大速度（最快），必须为0才能达到最快
    #define SERVO_ACC        0       // 加速度：0 = 最大加速度（最快启动），必须为0才能最快启动
    #define MOVEMENT_DELAY  1000     // 移动等待时间(ms)：5000ms = 5秒，确保舵机有足够时间完成移动
    
    // 角度转换函数（根据寄存器说明：位置单位是 0.087度）
    // 角度转寄存器值：角度 / 0.087
    // 寄存器值转角度：寄存器值 * 0.087
    auto degreeToReg = [](float degree) -> int {
        return (int)(degree / 0.087f);
    };
    auto regToDegree = [](int reg) -> float {
        return reg * 0.087f;
    };
    
    int cycle_count = 0;
    while (1) {
        cycle_count++;
        ESP_LOGI(TAG, "\n--- Cycle %d ---", cycle_count);
        
        // 移动到 0 度
        int target_pos_reg = degreeToReg(0);
        ESP_LOGI(TAG, "Moving servo to 0 degrees (reg value: %d, Speed: %d, ACC: %d)", target_pos_reg, SERVO_SPEED, SERVO_ACC);
        ret = servo.WritePosEx(SERVO_ID, target_pos_reg, SERVO_SPEED, SERVO_ACC);
        // 注意：Ack返回1表示成功，0表示失败（与通常的返回值逻辑相反）
        if (ret == 1) {
            ESP_LOGI(TAG, "Command sent successfully");
        } else {
            ESP_LOGW(TAG, "Command failed, error: %d", ret);
        }
        vTaskDelay(pdMS_TO_TICKS(MOVEMENT_DELAY));
        
        // 读取当前位置
        int pos_reg = servo.ReadPos(SERVO_ID);
        if (servo.getErr() == 0) {
            float pos_degree = regToDegree(pos_reg);
            ESP_LOGI(TAG, "Current position: %d (reg) = %.1f degrees", pos_reg, pos_degree);
        } else {
            ESP_LOGW(TAG, "Failed to read position, error: %d", servo.getErr());
        }
        
        // 移动到 90 度
        target_pos_reg = degreeToReg(90);
        ESP_LOGI(TAG, "Moving servo to 90 degrees (reg value: %d, Speed: %d, ACC: %d)", target_pos_reg, SERVO_SPEED, SERVO_ACC);
        ret = servo.WritePosEx(SERVO_ID, target_pos_reg, SERVO_SPEED, SERVO_ACC);
        // 注意：Ack返回1表示成功，0表示失败（与通常的返回值逻辑相反）
        if (ret == 1) {
            ESP_LOGI(TAG, "Command sent successfully");
        } else {
            ESP_LOGW(TAG, "Command failed, error: %d", ret);
        }
        vTaskDelay(pdMS_TO_TICKS(MOVEMENT_DELAY));
        
        // 读取当前位置
        pos_reg = servo.ReadPos(SERVO_ID);
        if (servo.getErr() == 0) {
            float pos_degree = regToDegree(pos_reg);
            ESP_LOGI(TAG, "Current position: %d (reg) = %.1f degrees", pos_reg, pos_degree);
        } else {
            ESP_LOGW(TAG, "Failed to read position, error: %d", servo.getErr());
        }
        
        // 移动到 180 度
        target_pos_reg = degreeToReg(180);
        ESP_LOGI(TAG, "Moving servo to 180 degrees (reg value: %d, Speed: %d, ACC: %d)", target_pos_reg, SERVO_SPEED, SERVO_ACC);
        ret = servo.WritePosEx(SERVO_ID, target_pos_reg, SERVO_SPEED, SERVO_ACC);
        // 注意：Ack返回1表示成功，0表示失败（与通常的返回值逻辑相反）
        if (ret == 1) {
            ESP_LOGI(TAG, "Command sent successfully");
        } else {
            ESP_LOGW(TAG, "Command failed, error: %d", ret);
        }
        vTaskDelay(pdMS_TO_TICKS(MOVEMENT_DELAY));
        
        // 读取当前位置
        pos_reg = servo.ReadPos(SERVO_ID);
        if (servo.getErr() == 0) {
            float pos_degree = regToDegree(pos_reg);
            ESP_LOGI(TAG, "Current position: %d (reg) = %.1f degrees", pos_reg, pos_degree);
        } else {
            ESP_LOGW(TAG, "Failed to read position, error: %d", servo.getErr());
        }
    }
}

