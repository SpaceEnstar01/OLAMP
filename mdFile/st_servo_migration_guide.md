# st_servo 移植到 xiaozhi-esp32 参考文档

## 概述

将 `/home/zexuan/esp/workspace/st_servo` 中实现的 lerobot 兼容舵机控制功能移植到 OLAMP 项目。

**核心目标**: 让 OLAMP 能够使用与 lerobot Python 端完全一致的角度转换公式控制舵机。

---

## 项目结构对比

| 功能 | st_servo 路径 | xiaozhi-esp32 路径 |
|------|--------------|-------------------|
| 舵机驱动库 | `main/st_servo/` | `main/servo/st_servo/` ✅ 已有 |
| 舵机封装类 | 无 (直接使用 SMS_STS) | `main/servo/servo.h/cc` ✅ 已有 |
| 校准数据 | `main/servo_calibration_data.h` | 需新增 |
| 转换函数 | `main/servo_calibration.cpp` | 需移植到 `servo.cc` |
| 动作数据 | `main/smile_test03.h` | 根据需求新增 |
| 参考副本 | - | `main/TEST/st_servo/` ✅ 已有 |

---

## 核心移植内容

### 1. 当前 xiaozhi-esp32 的转换公式 (需替换)

```cpp
// servo.cc 第154-159行 - 旧公式 (不兼容 lerobot)
int Servo::DegreeToReg(float degree) {
    return (int)(degree / 0.087f);
}

float Servo::RegToDegree(int reg) {
    return reg * 0.087f;
}
```

**问题**: 使用固定系数，不考虑校准数据，与 lerobot 不兼容。

### 2. 需要移植的 lerobot 兼容公式

```cpp
// st_servo/main/servo_calibration.cpp - 正确公式
int16_t angle_to_position(float angle, uint8_t servo_idx) {
    // 计算物理范围中点 (每个舵机不同)
    float mid = ((float)servo_range_min[servo_idx] + 
                 (float)servo_range_max[servo_idx]) / 2.0f;
    
    // 角度转位置 (与 lerobot 完全一致)
    float position = (angle * 4095.0f / 360.0f) + mid;
    
    // 限幅
    if (position < (float)servo_range_min[servo_idx]) {
        position = (float)servo_range_min[servo_idx];
    }
    if (position > (float)servo_range_max[servo_idx]) {
        position = (float)servo_range_max[servo_idx];
    }
    
    return (int16_t)position;
}
```

**公式对比**:

| 项目 | 旧公式 | lerobot 兼容公式 |
|------|--------|-----------------|
| 中心点 | 无 (隐式 0) | `(range_min + range_max) / 2` |
| 系数 | `1/0.087 ≈ 11.49` | `4095/360 ≈ 11.375` |
| 校准数据 | 不使用 | 使用 range_min/range_max |

---

## 移植方案

### 方案 A: 最小改动 (推荐)

在现有 `Servo` 类基础上添加校准支持。

#### 步骤 1: 添加校准数据结构

在 `servo.h` 中添加:

```cpp
struct ServoCalibration {
    uint16_t range_min;
    uint16_t range_max;
    int16_t homing_offset;  // 仅用于 EEPROM 写入
};
```

#### 步骤 2: 修改 Servo 类

```cpp
class Servo {
public:
    // 新增: 设置校准数据
    void SetCalibration(uint16_t range_min, uint16_t range_max);
    
    // 新增: 使用 lerobot 兼容公式的移动方法
    bool MoveToCalibrated(float degree, int speed = 0, int acc = 0);

private:
    ServoCalibration calibration_;
    bool has_calibration_;
    
    // 新增: lerobot 兼容转换
    int DegreeToRegCalibrated(float degree);
};
```

#### 步骤 3: 实现新方法

```cpp
void Servo::SetCalibration(uint16_t range_min, uint16_t range_max) {
    calibration_.range_min = range_min;
    calibration_.range_max = range_max;
    has_calibration_ = true;
}

int Servo::DegreeToRegCalibrated(float degree) {
    if (!has_calibration_) {
        // 回退到旧公式
        return DegreeToReg(degree);
    }
    
    float mid = ((float)calibration_.range_min + 
                 (float)calibration_.range_max) / 2.0f;
    float position = (degree * 4095.0f / 360.0f) + mid;
    
    if (position < (float)calibration_.range_min) {
        position = (float)calibration_.range_min;
    }
    if (position > (float)calibration_.range_max) {
        position = (float)calibration_.range_max;
    }
    
    return (int)position;
}

bool Servo::MoveToCalibrated(float degree, int speed, int acc) {
    if (!initialized_) return false;
    
    int target_pos_reg = DegreeToRegCalibrated(degree);
    int ret = servo_.WritePosEx(servo_id_, target_pos_reg, speed, acc);
    return (ret == 1);
}
```

---

### 方案 B: 多舵机控制器 (完整移植)

创建新的多舵机控制类，支持同步控制多个舵机。

#### 新建 `multi_servo.h`

```cpp
#ifndef MULTI_SERVO_H
#define MULTI_SERVO_H

#include "st_servo/SMS_STS.h"
#include <array>

struct ServoConfig {
    uint8_t id;
    uint16_t range_min;
    uint16_t range_max;
    int16_t homing_offset;
};

class MultiServo {
public:
    static constexpr int MAX_SERVOS = 5;
    
    MultiServo();
    
    bool Init(uart_port_t uart_num, int tx_pin, int rx_pin, int baud_rate);
    
    // 设置舵机配置
    void SetServoConfig(uint8_t index, const ServoConfig& config);
    
    // 同步移动所有舵机到指定角度
    bool SyncMoveTo(const float angles[], int count, int speed = 0, int acc = 0);
    
    // 单个舵机移动
    bool MoveTo(uint8_t index, float angle, int speed = 0, int acc = 0);
    
    // 播放动作序列
    bool PlayAction(const int16_t action[][MAX_SERVOS], int frame_count, 
                    int interval_ms = 50);

private:
    SMS_STS servo_;
    std::array<ServoConfig, MAX_SERVOS> configs_;
    int servo_count_;
    bool initialized_;
    
    int16_t AngleToPosition(float angle, uint8_t index);
};

#endif
```

---

## 校准数据来源

### 方法 1: 使用 Python 脚本生成

```bash
cd /home/zexuan/esp/workspace/st_servo/lelamp/record_esp32
python3 convert_calibration.py --lamp_id lamp03 \
    --output /home/zexuan/X/lab/xiaozhi-esp32/main/servo/servo_calibration_data.h
```

### 方法 2: 硬编码 (快速测试)

```cpp
// lamp03 校准数据
const ServoConfig lamp03_configs[5] = {
    {1, 795,  3379, -1437},  // base_yaw
    {2, 929,  3180,  1470},  // base_pitch
    {3, 878,  3064, -1302},  // elbow_pitch
    {4, 855,  3165,   725},  // wrist_roll
    {5, 193,  4012, -1259},  // wrist_pitch
};
```

---

## 文件复制清单

从 `st_servo` 复制到 `xiaozhi-esp32`:

| 源文件 | 目标位置 | 用途 |
|--------|----------|------|
| `main/servo_calibration_data.h` | `main/servo/` | 校准数据 |
| `lelamp/record_esp32/convert_calibration.py` | `scripts/` | 生成工具 |
| `main/smile_test03.h` | `main/servo/actions/` | 动作数据示例 |

**已存在** (无需复制):
- `main/st_servo/` → `main/servo/st_servo/` ✅
- 完整项目参考 → `main/TEST/st_servo/` ✅

---

## 集成到 OLAMP

### MCP 协议集成

在 `mcp_server.cc` 中添加舵机控制命令:

```cpp
// 示例: 通过 MCP 控制舵机
void handleServoCommand(const json& params) {
    int servo_id = params["servo_id"];
    float angle = params["angle"];
    
    // 使用 lerobot 兼容的转换
    multi_servo.MoveTo(servo_id - 1, angle);
}
```

### 语音控制集成

```cpp
// 示例: 语音触发动作
void playWaveAction() {
    multi_servo.PlayAction(wave_action, WAVE_FRAME_COUNT, 50);
}
```

---

## 验证步骤

1. **编译测试**: 确保新增代码编译通过
2. **单舵机测试**: 使用 `MoveToCalibrated()` 测试单个舵机
3. **多舵机同步**: 使用 `SyncMoveTo()` 测试同步控制
4. **动作回放**: 播放 `smile_test03` 验证与 lerobot 一致性

---

## 注意事项

1. **UART 冲突**: OLAMP 可能已使用某些 UART，需确认可用端口
2. **GPIO 分配**: 检查 TX/RX 引脚是否与其他功能冲突
3. **电源**: 多舵机同时运动时注意电源容量
4. **homing_offset**: 仅在首次初始化时写入舵机 EEPROM，后续不需要

---

## 参考文件路径

| 文件 | 路径 |
|------|------|
| 原始实现 | `/home/zexuan/esp/workspace/st_servo/main/` |
| 参考副本 | `/home/zexuan/X/lab/xiaozhi-esp32/main/TEST/st_servo/` |
| 目标位置 | `/home/zexuan/X/lab/xiaozhi-esp32/main/servo/` |
| 转换公式文档 | `st_servo/mdFile/lerobot_angle_conversion.md` |
| 调试记录 | `st_servo/mdFile/debugging_success_story.md` |
| 实现指南 | `st_servo/mdFile/esp32_lerobot_reproduction_guide.md` |

