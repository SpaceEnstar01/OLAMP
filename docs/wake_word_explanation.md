# 唤醒词机制说明

## 两种不同的唤醒词实现方式

### 1. WonderLLM 模块（你提到的文档）

**架构：**
- **CI1302 芯片**：负责唤醒词识别（语音识别侧）
- **ESP32S3 芯片**：负责音频播报（播报音频侧）

**特点：**
- 唤醒词**不在 ESP32S3 固件中**
- 唤醒词在 **CI1302 固件中**（通过启英泰伦语音AI平台制作）
- ESP32S3 只接收 CI1302 的识别结果，然后播放音频

**修改唤醒词的方式：**
1. **语音自学习**：通过语音指令修改（无需电脑）
2. **重新制作固件**：
   - 修改 CI1302 固件（在启英泰伦平台修改协议表）
   - 修改 ESP32S3 固件（替换播报音频文件）

---

### 2. 当前工程（xiaozhi-esp32）

**架构：**
- **ESP32 芯片**：同时负责唤醒词识别和音频处理

**特点：**
- 唤醒词**包含在 ESP32 固件中**
- 使用 **ESP-SR（Espressif Speech Recognition）** 库实现
- 唤醒词模型通过 `esp_srmodel_init("model")` 从固件中加载

**唤醒词实现方式：**

#### 方式一：AFE Wake Word（默认，ESP32-S3/P4）
```c
// 使用 AFE（Audio Front End）+ WakeNet
// 代码位置：main/audio/wake_words/afe_wake_word.cc
models_ = esp_srmodel_init("model");  // 从固件加载模型
```

#### 方式二：ESP Wake Word（ESP32-C3/C5/C6）
```c
// 使用 WakeNet
// 代码位置：main/audio/wake_words/esp_wake_word.cc
wakenet_model_ = esp_srmodel_init("model");  // 从固件加载模型
```

#### 方式三：Custom Wake Word（自定义唤醒词）
```c
// 使用 Multinet 命令词识别
// 代码位置：main/audio/wake_words/custom_wake_word.cc
models_ = esp_srmodel_init("model");  // 从固件加载模型
esp_mn_commands_add(1, CONFIG_CUSTOM_WAKE_WORD);  // 添加自定义唤醒词
```

**配置方式：**

在 `main/Kconfig.projbuild` 中配置：

```kconfig
config USE_CUSTOM_WAKE_WORD
    bool "Enable Custom Wake Word Detection"
    default n
    depends on (IDF_TARGET_ESP32S3 || IDF_TARGET_ESP32P4) && SPIRAM

config CUSTOM_WAKE_WORD
    string "Custom Wake Word"
    default "xiao tu dou"
    depends on USE_CUSTOM_WAKE_WORD
    help
        自定义唤醒词，中文用拼音表示，每个字之间用空格隔开

config CUSTOM_WAKE_WORD_DISPLAY
    string "Custom Wake Word Display"
    default "小土豆"
    depends on USE_CUSTOM_WAKE_WORD
```

**修改唤醒词的方式：**

1. **修改配置项**（仅适用于 Custom Wake Word）：
   - 修改 `CONFIG_CUSTOM_WAKE_WORD` 配置
   - 重新编译固件

2. **替换模型文件**（需要重新训练模型）：
   - 使用 ESP-SR 工具训练新的唤醒词模型
   - 将模型文件放入固件的模型目录
   - 重新编译固件

---

## 总结

### 你的困惑解答

**问题：** 当前工程文件编译的固件是否包含唤醒词？

**答案：** **是的，包含唤醒词！**

但是：
- **WonderLLM 模块**：唤醒词在 CI1302 固件中，不在 ESP32S3 固件中
- **xiaozhi-esp32 工程**：唤醒词在 ESP32 固件中（通过 ESP-SR 模型）

### 关键区别

| 项目 | 唤醒词位置 | 识别芯片 | 修改方式 |
|------|-----------|---------|---------|
| WonderLLM | CI1302 固件 | CI1302 | 修改 CI1302 固件或语音自学习 |
| xiaozhi-esp32 | ESP32 固件 | ESP32 | 修改配置或替换模型文件 |

### 如何确认你的项目类型

1. **检查硬件**：
   - 如果有 CI1302 芯片 → WonderLLM 模块架构
   - 如果只有 ESP32 芯片 → xiaozhi-esp32 架构

2. **检查代码**：
   - 如果代码中有 `esp_srmodel_init("model")` → xiaozhi-esp32 架构
   - 如果代码中有 UART 通信接收唤醒词结果 → 可能是 WonderLLM 架构

3. **检查配置文件**：
   - 如果有 `CONFIG_USE_CUSTOM_WAKE_WORD` 等配置 → xiaozhi-esp32 架构

---

## 参考资料

- [ESP-SR 官方文档](https://github.com/espressif/esp-sr)
- [自定义唤醒词教程](https://pcn7cs20v8cr.feishu.cn/wiki/CpQjwQsCJiQSWSkYEvrcxcbVnwh)
- [启英泰伦语音AI平台](https://platform.chipintelli.com/)

