Lamp Server 模块文件一览（简要）
================================

| 文件 | 主要功能（简化说明） |
|------|----------------------|
| `lamp_presets.h` | 定义通用 `LampPreset` 结构，声明获取/查找预设的接口。 |
| `lamp_presets.cc` | 从当前机械臂配置（如 `servo/calibration/lamp05.h`）构建预设表，实现 `GetLampPresets` / `FindLampPreset`。 |
| `lamp_mapping.h` | 定义高层动作类型 `LampActionType` 与 `LampCommand` 结构。 |
| `lamp_mapping.cc` | 将 `GO_PRESET` 动作解析为具体 5 关节角度（使用 `lamp_presets`）。 |
| `lamp_mcp_bridge.h` | 声明对外高层接口：`LampSendPreset` / `LampSendAngles`。 |
| `lamp_mcp_bridge.cc` | 实现上述接口：解析预设名或角度数组，并调用 `ServoManager::MoveToAngles` 执行动作。 |
| `servo/calibration/lamp05.h` | 机械臂 lamp05 的校准数据和高层预设 `kLamp05Presets`（home/horizontal/vertical 等）。 |
| `servo/servo_manager.h` | Servo 总管理入口，新增 `MoveToAngles` 提供“5 关节平滑移动”高层接口。 |
| `servo/servo_manager.cc` | 实现 `MoveToAngles`：使用固定步长线性插值和 `MultiServo::SyncMoveTo` 实现平滑过渡。 |

