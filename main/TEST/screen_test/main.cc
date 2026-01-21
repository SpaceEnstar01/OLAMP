#include "test_framework.h"
#include "test_utils.h"
#include "screen_test.h"
#include "board.h"
#include "display.h"
#include <esp_log.h>

#define TAG "ScreenTestMain"

extern "C" void app_main(void) {
    TestUtils::PrintTestTitle("Screen Module Test");
    
    // 初始化系统组件
    TestUtils::InitializeSystem();
    
    // 初始化测试框架
    TestFramework::GetInstance().Initialize();
    
    ESP_LOGI(TAG, "Getting Board instance...");
    auto& board = Board::GetInstance();
    
    ESP_LOGI(TAG, "Getting Display instance...");
    auto* display = board.GetDisplay();
    
    if (display == nullptr) {
        ESP_LOGE(TAG, "Display is null! Cannot run tests.");
        TestFramework::GetInstance().ReportResult("Display Initialization", false, "Display pointer is null");
        TestFramework::GetInstance().Cleanup();
        return;
    }
    
    ESP_LOGI(TAG, "Display initialized successfully");
    ESP_LOGI(TAG, "Display dimensions: %dx%d", display->width(), display->height());
    
    // 运行所有屏幕测试
    ScreenTest::RunAllTests(display);
    
    // 打印测试摘要
    TestFramework::GetInstance().Cleanup();
    
    ESP_LOGI(TAG, "Screen test program completed");
    
    // 保持运行，方便查看结果
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
















