#include "test_framework.h"
#include "test_utils.h"
#include "camera_test.h"
#include "board.h"
#include "camera.h"
#include <esp_log.h>

#define TAG "CameraTestMain"

extern "C" void app_main(void) {
    TestUtils::PrintTestTitle("Camera Module Test");
    
    // 初始化系统组件
    TestUtils::InitializeSystem();
    
    // 初始化测试框架
    TestFramework::GetInstance().Initialize();
    
    ESP_LOGI(TAG, "Getting Board instance...");
    auto& board = Board::GetInstance();
    
    ESP_LOGI(TAG, "Getting Camera instance...");
    auto* camera = board.GetCamera();
    
    if (camera == nullptr) {
        ESP_LOGE(TAG, "Camera is null! Cannot run tests.");
        TestFramework::GetInstance().ReportResult("Camera Initialization", false, "Camera pointer is null");
        TestFramework::GetInstance().Cleanup();
        return;
    }
    
    ESP_LOGI(TAG, "Camera initialized successfully");
    
    // 运行所有摄像头测试
    CameraTest::RunAllTests(camera);
    
    // 打印测试摘要
    TestFramework::GetInstance().Cleanup();
    
    ESP_LOGI(TAG, "Camera test program completed");
    
    // 保持运行，方便查看结果
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}






































