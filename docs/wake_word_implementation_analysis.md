# xiaozhi-esp32 唤醒词实现机制分析

## 核心结论

**是的，当前工程编译的固件包含唤醒词。** 唤醒词模型直接嵌入在 ESP32 固件中，通过 ESP-SR（Espressif Speech Recognition）库实现本地离线识别。

## 实现架构

### 1. 模型加载机制
唤醒词模型在固件编译时打包，运行时通过 `esp_srmodel_init("model")` 从固件内部加载。模型文件存储在 ESP-IDF 的模型管理系统中，随固件一起烧录到设备。

### 2. 三种实现模式
根据硬件平台和配置，支持三种唤醒词实现：

- **AFE Wake Word**（ESP32-S3/P4，默认）：使用 AFE（Audio Front End）+ WakeNet，支持降噪和回声消除
- **ESP Wake Word**（ESP32-C3/C5/C6）：使用轻量级 WakeNet，适合资源受限平台
- **Custom Wake Word**（ESP32-S3/P4）：使用 Multinet 命令词识别，支持自定义唤醒词配置

### 3. 实时检测流程

```
音频输入 → AudioInputTask → wake_word_->Feed() → 模型检测 → 回调触发
```

- **音频采集**：`AudioInputTask` 任务持续读取麦克风数据（16kHz，单声道）
- **数据喂入**：按模型要求的 chunksize 将音频数据喂入唤醒词检测器
- **模型推理**：ESP-SR 库在 ESP32 上实时运行神经网络模型进行识别
- **事件触发**：检测到唤醒词后，通过回调函数 `OnWakeWordDetected()` 通知应用层

### 4. 代码关键路径

```cpp
// 初始化（audio_service.cc:53-58）
#if CONFIG_USE_AFE_WAKE_WORD
    wake_word_ = std::make_unique<AfeWakeWord>();
#elif CONFIG_USE_ESP_WAKE_WORD
    wake_word_ = std::make_unique<EspWakeWord>();
#elif CONFIG_USE_CUSTOM_WAKE_WORD
    wake_word_ = std::make_unique<CustomWakeWord>();
#endif

// 模型加载（esp_wake_word.cc:20）
wakenet_model_ = esp_srmodel_init("model");  // 从固件加载

// 实时检测（audio_service.cc:248-256）
if (bits & AS_EVENT_WAKE_WORD_RUNNING) {
    std::vector<int16_t> data;
    int samples = wake_word_->GetFeedSize();
    if (ReadAudioData(data, 16000, samples)) {
        wake_word_->Feed(data);  // 喂入音频数据
    }
}
```

## 与 WonderLLM 的区别

| 特性 | xiaozhi-esp32 | WonderLLM |
|------|---------------|-----------|
| 识别芯片 | ESP32 自身 | CI1302 独立芯片 |
| 模型位置 | ESP32 固件内 | CI1302 固件内 |
| 修改方式 | 重新编译固件或配置 | 修改 CI1302 固件或语音自学习 |
| 架构 | 单芯片集成 | 双芯片分离 |

## 总结

xiaozhi-esp32 采用**嵌入式 AI 方案**，唤醒词模型直接运行在 ESP32 上，无需外部芯片。模型在编译时打包进固件，运行时从 Flash 加载到内存进行实时推理，实现完全离线的唤醒词检测。

