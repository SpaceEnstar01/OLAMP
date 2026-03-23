# 关节 1 / 关节 5 语音控制暂时注释说明

## 记录

- **时间**：为便于测试关节 2/3 等新逻辑，暂时关闭关节 1、5 的语音入口。
- **状态**：关节 1（底座左右转）、关节 5（灯头俯仰）已成功完成语音测试，逻辑与 MCP 工具均保留，仅**不注册**到 MCP，使上位机无法调用。
- **修改位置**：`main/mcp_server.cc`，将 `self.lamp.joint1.relative_move` 与 `self.lamp.joint5.relative_move` 两段 `AddTool(...)` 用 `/* ... */` 整体注释。

## 快速恢复方案

1. 打开 `main/mcp_server.cc`。
2. 搜索 `暂时注释：关节1/5`，找到注释块。
3. 删除该注释块**最外层的** `/*` 和 `*/`（即恢复两段 `AddTool` 代码，保留其内的 `// Relative move for lamp joint1...` 等注释）。
4. 保存后执行：
   ```bash
   cd /home/zexuan/X/lab/xiaozhi-esp32
   source $HOME/esp/esp-idf/export.sh
   idf.py build
   idf.py -p /dev/ttyUSB0 flash
   ```
5. 烧录完成后，语音“右转一点点/左转一点点”“抬高一点/放低一点”将再次生效。

## 说明

- `LampMoveJoint1Relative`、`LampMoveJoint5Relative` 及 `lamp_mcp_bridge` 未改动，仅 MCP 未注册上述两个工具。
- 恢复时只需取消 `mcp_server.cc` 中对应的一对 `/*`、`*/` 即可。
