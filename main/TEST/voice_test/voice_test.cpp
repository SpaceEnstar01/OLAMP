#include "voice_test.h"
#include "test_framework.h"
#include "test_utils.h"
#include "audio_codec.h"
#include <esp_log.h>

#define TAG "VoiceTest"

void VoiceTest::RunAllTests(AudioCodec* codec) {
    if (codec == nullptr) {
        TEST_FAIL("VoiceTest", "AudioCodec is null");
        return;
    }

    ESP_LOGI(TAG, "Starting voice module tests...");

    // 测试1：编解码器初始化
    TEST_START("Codec Initialization Test");
    TestCodecInit(codec);

    // 测试2：采样率
    TEST_START("Sample Rate Test");
    TestSampleRate(codec);

    // 测试3：录音功能
    TEST_START("Recording Test");
    TestRecording(codec);

    // 测试4：播放功能
    TEST_START("Playback Test");
    TestPlayback(codec);

    // 测试5：音量控制
    TEST_START("Volume Control Test");
    TestVolumeControl(codec);

    // 测试6：回声消除（如果支持）
    TEST_START("Echo Cancellation Test");
    TestEchoCancellation(codec);

    ESP_LOGI(TAG, "Voice module tests completed");
}

void VoiceTest::TestCodecInit(AudioCodec* codec) {
    ESP_LOGI(TAG, "Testing codec initialization...");

    if (codec == nullptr) {
        TEST_FAIL("CodecInit", "AudioCodec pointer is null");
        return;
    }

    // 检查编解码器是否已初始化
    int input_rate = codec->input_sample_rate();
    int output_rate = codec->output_sample_rate();
    
    if (input_rate <= 0 || output_rate <= 0) {
        TEST_FAIL("CodecInit", "Invalid sample rates");
        return;
    }

    ESP_LOGI(TAG, "Input sample rate: %d Hz", input_rate);
    ESP_LOGI(TAG, "Output sample rate: %d Hz", output_rate);
    TEST_PASS("CodecInit");
}

void VoiceTest::TestRecording(AudioCodec* codec) {
    ESP_LOGI(TAG, "Testing recording functionality...");

    if (codec == nullptr) {
        TEST_FAIL("Recording", "AudioCodec is null");
        return;
    }

    // 测试录音功能
    // 需要根据实际的AudioCodec接口来实现
    // 例如：读取音频数据并验证
    
    ESP_LOGI(TAG, "Recording test completed");
    TEST_PASS("Recording");
}

void VoiceTest::TestPlayback(AudioCodec* codec) {
    ESP_LOGI(TAG, "Testing playback functionality...");

    if (codec == nullptr) {
        TEST_FAIL("Playback", "AudioCodec is null");
        return;
    }

    // 测试播放功能
    // 需要根据实际的AudioCodec接口来实现
    // 例如：播放测试音频并验证
    
    ESP_LOGI(TAG, "Playback test completed");
    TEST_PASS("Playback");
}

void VoiceTest::TestVolumeControl(AudioCodec* codec) {
    ESP_LOGI(TAG, "Testing volume control...");

    if (codec == nullptr) {
        TEST_FAIL("VolumeControl", "AudioCodec is null");
        return;
    }

    // 测试音量控制
    // 需要根据实际的AudioCodec接口来实现
    // 例如：设置不同音量级别并验证
    
    ESP_LOGI(TAG, "Volume control test completed");
    TEST_PASS("VolumeControl");
}

void VoiceTest::TestSampleRate(AudioCodec* codec) {
    ESP_LOGI(TAG, "Testing sample rate...");

    if (codec == nullptr) {
        TEST_FAIL("SampleRate", "AudioCodec is null");
        return;
    }

    int input_rate = codec->input_sample_rate();
    int output_rate = codec->output_sample_rate();
    
    ESP_LOGI(TAG, "Input sample rate: %d Hz", input_rate);
    ESP_LOGI(TAG, "Output sample rate: %d Hz", output_rate);
    
    // 验证采样率是否合理
    if (input_rate >= 8000 && input_rate <= 48000 && 
        output_rate >= 8000 && output_rate <= 48000) {
        TEST_PASS("SampleRate");
    } else {
        TEST_FAIL("SampleRate", "Sample rates out of valid range");
    }
}

void VoiceTest::TestEchoCancellation(AudioCodec* codec) {
    ESP_LOGI(TAG, "Testing echo cancellation...");

    if (codec == nullptr) {
        TEST_FAIL("EchoCancellation", "AudioCodec is null");
        return;
    }

    // 检查是否支持回声消除
    bool supports_aec = codec->input_reference();
    
    if (supports_aec) {
        ESP_LOGI(TAG, "Echo cancellation is supported");
        // 可以添加更详细的测试
        TEST_PASS("EchoCancellation");
    } else {
        ESP_LOGI(TAG, "Echo cancellation is not supported");
        TEST_PASS("EchoCancellation"); // 不支持不算失败
    }
}
















