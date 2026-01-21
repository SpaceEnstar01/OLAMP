#include "test_utils.h"
#include <esp_log.h>
#include <esp_err.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <esp_event.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cstdio>
#include <cstring>

#define TAG "TestUtils"

void TestUtils::InitializeSystem() {
    ESP_LOGI(TAG, "Initializing system components...");

    // Initialize the default event loop
    esp_err_t ret = esp_event_loop_create_default();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create event loop: %s", esp_err_to_name(ret));
        return;
    }

    // Initialize NVS flash
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Erasing NVS flash to fix corruption");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "System components initialized successfully");
}

void TestUtils::DelayMs(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void TestUtils::DelaySeconds(uint32_t seconds) {
    vTaskDelay(pdMS_TO_TICKS(seconds * 1000));
}

void TestUtils::PrintTestInfo(const std::string& module, const std::string& info) {
    ESP_LOGI(module.c_str(), "%s", info.c_str());
}

void TestUtils::PrintTestError(const std::string& module, const std::string& error) {
    ESP_LOGE(module.c_str(), "%s", error.c_str());
}

bool TestUtils::CheckGpioState(gpio_num_t gpio, int level) {
    int current_level = gpio_get_level(gpio);
    return current_level == level;
}

void TestUtils::SetGpioOutput(gpio_num_t gpio, int level) {
    gpio_reset_pin(gpio);
    gpio_set_direction(gpio, GPIO_MODE_OUTPUT);
    gpio_set_level(gpio, level);
}

std::string TestUtils::FormatResult(bool passed, const std::string& message) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "[%s] %s", 
             passed ? "PASS" : "FAIL", message.c_str());
    return std::string(buffer);
}

bool TestUtils::CheckEspError(esp_err_t err, const std::string& operation) {
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "%s failed: %s", operation.c_str(), esp_err_to_name(err));
        return false;
    }
    return true;
}

void TestUtils::PrintSeparator(int width) {
    std::string separator(width, '=');
    ESP_LOGI(TAG, "%s", separator.c_str());
}

void TestUtils::PrintTestTitle(const std::string& title) {
    PrintSeparator();
    ESP_LOGI(TAG, "  %s", title.c_str());
    PrintSeparator();
}
















