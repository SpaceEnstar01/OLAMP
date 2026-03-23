# ESP32 与电脑通信控制机械手方案

## 需求场景

- ESP32 模块进行语音识别
- 电脑控制机械手
- 需要让 ESP32 的语音指令能够控制电脑上的机械手

## 推荐方案

### 方案一：WebSocket 本地服务器（推荐）⭐

**架构：**
```
ESP32 (语音识别) → WebSocket → 电脑本地服务器 → 机械手控制
```

**优点：**
- 项目已支持 WebSocket 协议
- 实时性好，延迟低
- 支持双向通信
- 可以传输结构化数据（JSON）

**实现步骤：**

1. **电脑端：启动本地 WebSocket 服务器**
```python
# robot_server.py
import asyncio
import websockets
import json

async def handle_client(websocket, path):
    print("ESP32 已连接")
    async for message in websocket:
        data = json.loads(message)
        
        # 处理语音指令
        if data.get("type") == "mcp":
            payload = data.get("payload", {})
            if payload.get("method") == "tools/call":
                tool_name = payload.get("params", {}).get("name", "")
                args = payload.get("params", {}).get("arguments", {})
                
                # 根据工具名称控制机械手
                if tool_name == "robot.arm.move":
                    control_robot_arm(args)
                elif tool_name == "robot.arm.grasp":
                    control_robot_grasp(args)

async def control_robot_arm(args):
    # 控制机械手运动
    x = args.get("x", 0)
    y = args.get("y", 0)
    z = args.get("z", 0)
    # 调用机械手控制代码
    print(f"控制机械手移动到: ({x}, {y}, {z})")

start_server = websockets.serve(handle_client, "localhost", 8765)
asyncio.get_event_loop().run_until_complete(start_server)
asyncio.get_event_loop().run_forever()
```

2. **ESP32 端：配置连接到本地服务器**
```cpp
// 在 board 初始化代码中添加 MCP 工具
void InitializeRobotArmTools() {
    auto& mcp_server = McpServer::GetInstance();
    
    // 添加机械手控制工具
    mcp_server.AddTool("robot.arm.move",
        "控制机械手移动到指定位置",
        PropertyList({
            Property("x", kPropertyTypeInteger, -100, 100),
            Property("y", kPropertyTypeInteger, -100, 100),
            Property("z", kPropertyTypeInteger, -100, 100)
        }),
        [](const PropertyList& properties) -> ReturnValue {
            int x = properties["x"].value<int>();
            int y = properties["y"].value<int>();
            int z = properties["z"].value<int>();
            
            // 通过 MCP 协议发送到服务器
            // 服务器会接收并控制机械手
            return "机械手移动到 (" + std::to_string(x) + ", " + 
                   std::to_string(y) + ", " + std::to_string(z) + ")";
        });
    
    mcp_server.AddTool("robot.arm.grasp",
        "控制机械手抓取/释放",
        PropertyList({
            Property("action", kPropertyTypeString)  // "grasp" 或 "release"
        }),
        [](const PropertyList& properties) -> ReturnValue {
            std::string action = properties["action"].value<std::string>();
            return "机械手" + (action == "grasp" ? "抓取" : "释放");
        });
}
```

3. **配置 WebSocket 连接到本地服务器**
- 在设备配网时，设置 WebSocket URL 为：`ws://电脑IP:8765`
- 或修改 `settings` 中的 `websocket.url` 配置

---

### 方案二：串口通信（最简单）⭐

**架构：**
```
ESP32 (语音识别) → UART 串口 → 电脑串口程序 → 机械手控制
```

**优点：**
- 实现简单，无需网络
- 延迟最低
- 稳定可靠

**实现步骤：**

1. **ESP32 端：添加串口发送工具**
```cpp
// 在 board 初始化代码中
#include "driver/uart.h"

void InitializeUartForRobot() {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, GPIO_NUM_17, GPIO_NUM_18, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_1, 1024, 0, 0, NULL, 0);
}

void SendRobotCommand(const std::string& command) {
    uart_write_bytes(UART_NUM_1, command.c_str(), command.length());
    uart_write_bytes(UART_NUM_1, "\n", 1);
}

// 在 MCP 工具中调用
mcp_server.AddTool("robot.arm.move",
    "控制机械手移动",
    PropertyList({...}),
    [](const PropertyList& properties) -> ReturnValue {
        // 发送串口命令
        std::string cmd = "MOVE " + std::to_string(x) + " " + 
                         std::to_string(y) + " " + std::to_string(z);
        SendRobotCommand(cmd);
        return true;
    });
```

2. **电脑端：串口接收程序**
```python
# robot_serial.py
import serial
import time

ser = serial.Serial('COM3', 115200)  # Windows
# ser = serial.Serial('/dev/ttyUSB0', 115200)  # Linux

while True:
    if ser.in_waiting:
        line = ser.readline().decode('utf-8').strip()
        print(f"收到命令: {line}")
        
        # 解析命令并控制机械手
        if line.startswith("MOVE"):
            parts = line.split()
            x, y, z = float(parts[1]), float(parts[2]), float(parts[3])
            control_robot_arm(x, y, z)
        elif line == "GRASP":
            control_robot_grasp()
        elif line == "RELEASE":
            control_robot_release()
```

---

### 方案三：WiFi TCP/UDP 通信

**架构：**
```
ESP32 (语音识别) → TCP/UDP → 电脑本地服务器 → 机械手控制
```

**实现：**
- 使用 ESP32 的 TCP/UDP 客户端功能
- 电脑端运行 TCP/UDP 服务器
- 类似 WebSocket，但更底层

---

## 推荐选择

| 方案 | 优点 | 缺点 | 适用场景 |
|------|------|------|---------|
| **WebSocket** | 已集成，双向通信，结构化数据 | 需要网络配置 | 推荐，功能完整 |
| **串口** | 最简单，延迟最低，稳定 | 需要物理连接 | 调试阶段，简单场景 |
| **TCP/UDP** | 灵活，可自定义协议 | 需要自己实现协议 | 特殊需求 |

## 快速开始（WebSocket 方案）

1. **电脑端启动服务器：**
```bash
python robot_server.py
```

2. **ESP32 配置：**
   - 在配网界面设置 WebSocket URL：`ws://你的电脑IP:8765`
   - 或修改代码中的默认配置

3. **添加 MCP 工具：**
   - 在 `main/boards/你的板子/你的板子.cc` 的 `InitializeTools()` 中添加机械手控制工具

4. **测试：**
   - 对 ESP32 说："让机械手移动到 (10, 20, 30)"
   - 电脑端会收到指令并控制机械手

## 注意事项

1. **电脑 IP 地址**：确保 ESP32 和电脑在同一局域网
2. **防火墙**：确保电脑防火墙允许 WebSocket/TCP 连接
3. **端口占用**：确保端口未被占用
4. **实时性**：WebSocket 延迟通常 < 50ms，满足实时控制需求

