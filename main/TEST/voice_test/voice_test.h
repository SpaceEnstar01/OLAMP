#ifndef VOICE_TEST_H
#define VOICE_TEST_H

#include "audio_codec.h"

/**
 * @brief 语音模块测试类
 * 
 * 提供各种音频编解码功能测试
 */
class VoiceTest {
public:
    /**
     * @brief 运行所有语音测试
     * @param codec AudioCodec指针
     */
    static void RunAllTests(AudioCodec* codec);

    /**
     * @brief 测试音频编解码器初始化
     * @param codec AudioCodec指针
     */
    static void TestCodecInit(AudioCodec* codec);

    /**
     * @brief 测试录音功能
     * @param codec AudioCodec指针
     */
    static void TestRecording(AudioCodec* codec);

    /**
     * @brief 测试播放功能
     * @param codec AudioCodec指针
     */
    static void TestPlayback(AudioCodec* codec);

    /**
     * @brief 测试音量控制
     * @param codec AudioCodec指针
     */
    static void TestVolumeControl(AudioCodec* codec);

    /**
     * @brief 测试采样率
     * @param codec AudioCodec指针
     */
    static void TestSampleRate(AudioCodec* codec);

    /**
     * @brief 测试回声消除（如果支持）
     * @param codec AudioCodec指针
     */
    static void TestEchoCancellation(AudioCodec* codec);

private:
    VoiceTest() = default;
};

#endif // VOICE_TEST_H





































