#include "test_framework.h"
#include "test_utils.h"
#include "voice_test.h"
#include "board.h"
#include "audio_codec.h"
#include <esp_log.h>

#define TAG "VoiceTestMain"

extern "C" void app_main(void) {
    TestUtils::PrintTestTitle("Voice Module Test");
    
    // 初始化系统组件
    TestUtils::InitializeSystem();
    
    // 初始化测试框架
    TestFramework::GetInstance().Initialize();
    
    ESP_LOGI(TAG, "Getting Board instance...");
    auto& board = Board::GetInstance();
    
    ESP_LOGI(TAG, "Getting AudioCodec instance...");
    auto* codec = board.GetAudioCodec();
    
    if (codec == nullptr) {
        ESP_LOGE(TAG, "AudioCodec is null! Cannot run tests.");
        TestFramework::GetInstance().ReportResult("AudioCodec Initialization", false, "AudioCodec pointer is null");
        TestFramework::GetInstance().Cleanup();
        return;
    }
    
    ESP_LOGI(TAG, "AudioCodec initialized successfully");
    ESP_LOGI(TAG, "Input sample rate: %d Hz", codec->input_sample_rate());
    ESP_LOGI(TAG, "Output sample rate: %d Hz", codec->output_sample_rate());
    
    // 启动编解码器
    codec->Start();
    ESP_LOGI(TAG, "AudioCodec started");
    
    // 运行所有语音测试
    VoiceTest::RunAllTests(codec);
    
    // 打印测试摘要
    TestFramework::GetInstance().Cleanup();
    
    ESP_LOGI(TAG, "Voice test program completed");
    
    // 保持运行，方便查看结果
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
















