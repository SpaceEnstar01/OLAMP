#include "lamp_led.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/ledc.h>
#include <driver/gpio.h>

#define TAG "TestLampLed"

// ============================================
// 测试配置（参考成功代码）
// ============================================
#define TEST_GPIO           21      // GPIO 21
#define TEST_FREQ_HZ        5000    // 5kHz
#define TEST_RESOLUTION     8       // 8位分辨率 (0~255)
#define TEST_TIMER          LEDC_TIMER_1
#define TEST_MODE           LEDC_LOW_SPEED_MODE
#define TEST_CHANNEL        LEDC_CHANNEL_1
#define TEST_DUTY_MAX       ((1 << TEST_RESOLUTION) - 1)  // 255

// 测试模式：亮5秒，灭1秒，循环
#define TEST_ON_MS  5000
#define TEST_OFF_MS 1000

static TaskHandle_t s_test_task_handle = NULL;
static bool s_test_initialized = false;

/**
 * @brief 初始化测试LEDC（独立于LampLed类）
 */
static int test_ledc_init(void) {
    if (s_test_initialized) {
        ESP_LOGW(TAG, "Test LEDC already initialized");
        return 0;
    }

    // 重置GPIO
    gpio_reset_pin((gpio_num_t)TEST_GPIO);
    vTaskDelay(pdMS_TO_TICKS(10));

    // 配置 LEDC 定时器
    ledc_timer_config_t timer_conf = {};
    timer_conf.speed_mode      = TEST_MODE;
    timer_conf.timer_num       = TEST_TIMER;
    timer_conf.duty_resolution = (ledc_timer_bit_t)TEST_RESOLUTION;
    timer_conf.freq_hz         = TEST_FREQ_HZ;
    timer_conf.clk_cfg         = LEDC_AUTO_CLK;

    esp_err_t err = ledc_timer_config(&timer_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LEDC timer config failed: %s", esp_err_to_name(err));
        return -1;
    }

    // 配置 LEDC 通道
    ledc_channel_config_t ch_conf = {};
    ch_conf.speed_mode = TEST_MODE;
    ch_conf.channel    = TEST_CHANNEL;
    ch_conf.timer_sel  = TEST_TIMER;
    ch_conf.intr_type  = LEDC_INTR_DISABLE;
    ch_conf.gpio_num   = (gpio_num_t)TEST_GPIO;
    ch_conf.duty       = 0;  // 初始关灯
    ch_conf.hpoint     = 0;

    err = ledc_channel_config(&ch_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LEDC channel config failed: %s", esp_err_to_name(err));
        return -1;
    }

    s_test_initialized = true;
    ESP_LOGI(TAG, "Test LEDC initialized on GPIO %d (freq: %d Hz, resolution: %d bit, timer: %d, channel: %d)",
             TEST_GPIO, TEST_FREQ_HZ, TEST_RESOLUTION, TEST_TIMER, TEST_CHANNEL);

    return 0;
}

/**
 * @brief 测试任务：亮5秒 → 灭1秒 循环（直接操作LEDC，不通过LampLed类）
 */
static void test_task(void* arg) {
    ESP_LOGI(TAG, "=== Test task started: ON=%d ms, OFF=%d ms ===", TEST_ON_MS, TEST_OFF_MS);
    
    // 初始化LEDC
    if (test_ledc_init() != 0) {
        ESP_LOGE(TAG, "Failed to initialize test LEDC, exiting");
        vTaskDelete(NULL);
        return;
    }
    
    // 等待一小段时间，确保系统稳定
    vTaskDelay(pdMS_TO_TICKS(500));
    
    while (1) {
        // 亮灯（直接操作LEDC，参考成功代码）
        ESP_LOGI(TAG, "[TEST] Turning ON (duty=%d)", TEST_DUTY_MAX);
        ledc_set_duty(TEST_MODE, TEST_CHANNEL, TEST_DUTY_MAX);
        ledc_update_duty(TEST_MODE, TEST_CHANNEL);
        vTaskDelay(pdMS_TO_TICKS(TEST_ON_MS));
        
        // 灭灯（直接操作LEDC，参考成功代码）
        ESP_LOGI(TAG, "[TEST] Turning OFF (duty=0)");
        ledc_set_duty(TEST_MODE, TEST_CHANNEL, 0);
        ledc_update_duty(TEST_MODE, TEST_CHANNEL);
        vTaskDelay(pdMS_TO_TICKS(TEST_OFF_MS));
    }
}

/**
 * @brief 启动LED灯测试（独立测试，不依赖LampLed类）
 * @param lamp_led 参数保留以兼容接口，但实际不使用
 * 
 * 功能：创建一个FreeRTOS任务，使用成功代码的配置直接操作LEDC
 * 用于验证硬件连接和LEDC配置是否正确
 */
void StartLampLedTest(LampLed* lamp_led) {
    (void)lamp_led;  // 不使用，但保留参数以兼容接口
    
    ESP_LOGI(TAG, "StartLampLedTest called (using independent test, not LampLed class)");
    
    if (s_test_task_handle != NULL) {
        ESP_LOGW(TAG, "Test task already running, stopping old task first");
        vTaskDelete(s_test_task_handle);
        s_test_task_handle = NULL;
    }
    
    // 创建测试任务（不传递lamp_led参数）
    ESP_LOGI(TAG, "Creating independent test task...");
    BaseType_t ret = xTaskCreate(
        test_task,
        "lamp_test",
        2048,              // 栈大小
        NULL,              // 不使用参数
        5,                 // 优先级
        &s_test_task_handle
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create test task, ret=%d", ret);
    } else {
        ESP_LOGI(TAG, "Test task created successfully, handle=%p", s_test_task_handle);
    }
}

/**
 * @brief 停止LED灯测试
 */
void StopLampLedTest(void) {
    if (s_test_task_handle != NULL) {
        vTaskDelete(s_test_task_handle);
        s_test_task_handle = NULL;
        s_test_initialized = false;
        ESP_LOGI(TAG, "Test task stopped");
    }
}
