# 语音识别和LLM语义理解 - 代码分析

## 重要说明

**关键发现：** 语音识别（STT）和LLM语义理解**都在服务器端完成**，设备端只负责：
1. 发送音频数据到服务器
2. 接收服务器返回的STT文本结果
3. 接收服务器返回的MCP工具调用请求

设备端**没有**关键词识别、prompt处理或LLM API调用的代码。

---

## 完整流程（设备端 + 服务器端）

```
[设备端] 用户说"拍一张照片"
    ↓
[设备端] 麦克风采集音频 → I2S → AudioCodec
    ↓
[设备端] AudioService编码为Opus → 发送到服务器
    ↓
[服务器端] 接收Opus音频 → 解码 → STT语音识别
    ↓
[服务器端] STT结果："拍一张照片"
    ↓
[服务器端] LLM语义理解（识别意图：需要拍照）
    ↓
[服务器端] LLM调用工具：self.camera.take_photo
    ↓
[服务器端] 发送MCP工具调用请求到设备
    ↓
[设备端] 接收MCP消息 → 执行拍照
```

---

## 设备端代码（发送音频）

### 1. 音频采集和编码

**文件：** `main/audio/audio_service.cc`

```cpp
// AudioInputTask 持续读取麦克风数据
void AudioService::AudioInputTask() {
    while (true) {
        // 读取原始PCM数据
        std::vector<int16_t> data;
        int samples = audio_processor_->GetFeedSize();
        if (samples > 0) {
            if (ReadAudioData(data, 16000, samples)) {
                // 处理音频（AEC、VAD等）
                audio_processor_->Feed(std::move(data));
            }
        }
    }
}

// 处理后的音频被编码为Opus，放入发送队列
// audio_encode_queue_ → Opus编码 → audio_send_queue_
```

### 2. 发送音频到服务器

**文件：** `main/application.cc:552-558`

```cpp
if (bits & MAIN_EVENT_SEND_AUDIO) {
    while (auto packet = audio_service_.PopPacketFromSendQueue()) {
        if (!protocol_->SendAudio(std::move(packet))) {
            break;
        }
    }
}
```

**文件：** `main/protocols/websocket_protocol.cc:28-58`

```cpp
bool WebsocketProtocol::SendAudio(std::unique_ptr<AudioStreamPacket> packet) {
    if (websocket_ == nullptr || !websocket_->IsConnected()) {
        return false;
    }

    if (version_ == 2) {
        // 使用BinaryProtocol2格式
        std::string serialized;
        serialized.resize(sizeof(BinaryProtocol2) + packet->payload.size());
        auto bp2 = (BinaryProtocol2*)serialized.data();
        bp2->version = htons(version_);
        bp2->type = 0;  // 0表示音频
        bp2->timestamp = htonl(packet->timestamp);
        bp2->payload_size = htonl(packet->payload.size());
        memcpy(bp2->payload, packet->payload.data(), packet->payload.size());
        
        return websocket_->Send(serialized.data(), serialized.size(), true);
    }
    // ... 其他版本处理
}
```

---

## 设备端代码（接收STT结果）

### 接收STT文本

**文件：** `main/application.cc:436-443`

```cpp
else if (strcmp(type->valuestring, "stt") == 0) {
    auto text = cJSON_GetObjectItem(root, "text");
    if (cJSON_IsString(text)) {
        ESP_LOGI(TAG, ">> %s", text->valuestring);  // 打印："拍一张照片"
        Schedule([this, display, message = std::string(text->valuestring)]() {
            display->SetChatMessage("user", message.c_str());  // 显示到屏幕
        });
    }
}
```

**关键点：**
- 服务器发送：`{"type": "stt", "text": "拍一张照片"}`
- 设备端接收并显示，**不做任何语义理解**

---

## 设备端代码（接收MCP工具调用）

### 接收MCP消息

**文件：** `main/application.cc:451-455`

```cpp
else if (strcmp(type->valuestring, "mcp") == 0) {
    auto payload = cJSON_GetObjectItem(root, "payload");
    if (cJSON_IsObject(payload)) {
        McpServer::GetInstance().ParseMessage(payload);
    }
}
```

### 解析工具调用

**文件：** `main/mcp_server.cc:154-238`

```cpp
void McpServer::ParseMessage(const cJSON* json) {
    // 检查JSONRPC版本
    auto version = cJSON_GetObjectItem(json, "jsonrpc");
    if (version == nullptr || strcmp(version->valuestring, "2.0") != 0) {
        return;
    }
    
    // 检查方法
    auto method = cJSON_GetObjectItem(json, "method");
    auto method_str = std::string(method->valuestring);
    
    if (method_str == "tools/call") {
        // 解析工具调用请求
        auto params = cJSON_GetObjectItem(json, "params");
        auto tool_name = cJSON_GetObjectItem(params, "name");
        auto tool_arguments = cJSON_GetObjectItem(params, "arguments");
        
        // 执行工具调用
        DoToolCall(id_int, std::string(tool_name->valuestring), 
                   tool_arguments, stack_size);
    }
}
```

**服务器发送的MCP消息示例：**
```json
{
  "type": "mcp",
  "payload": {
    "jsonrpc": "2.0",
    "method": "tools/call",
    "params": {
      "name": "self.camera.take_photo",
      "arguments": {
        "question": "用户想要拍照"
      }
    },
    "id": 1
  }
}
```

---

## 服务器端处理（推测，不在设备代码中）

### 1. STT语音识别

服务器端应该有这样的处理：
```python
# 伪代码，服务器端实现
def handle_audio_stream(opus_audio):
    # 1. 解码Opus音频
    pcm_audio = opus_decode(opus_audio)
    
    # 2. 语音识别（STT）
    text = stt_model.transcribe(pcm_audio)  # 返回："拍一张照片"
    
    # 3. 发送STT结果到设备
    send_to_device({
        "type": "stt",
        "text": text
    })
    
    return text
```

### 2. LLM语义理解

服务器端应该有这样的处理：
```python
# 伪代码，服务器端实现
def handle_user_text(text):
    # 1. 构建LLM Prompt
    system_prompt = """
    你是一个智能助手，可以控制设备。
    可用的工具：
    - self.camera.take_photo: 拍照
    - self.audio_speaker.set_volume: 设置音量
    ...
    """
    
    user_message = text  # "拍一张照片"
    
    # 2. 调用LLM API（如OpenAI、Claude等）
    response = llm_api.chat_completion(
        messages=[
            {"role": "system", "content": system_prompt},
            {"role": "user", "content": user_message}
        ],
        tools=get_available_tools()  # 包含self.camera.take_photo
    )
    
    # 3. LLM返回工具调用
    if response.tool_calls:
        for tool_call in response.tool_calls:
            if tool_call.name == "self.camera.take_photo":
                # 4. 发送MCP工具调用到设备
                send_mcp_tool_call_to_device({
                    "jsonrpc": "2.0",
                    "method": "tools/call",
                    "params": {
                        "name": "self.camera.take_photo",
                        "arguments": {
                            "question": "用户想要拍照"
                        }
                    },
                    "id": 1
                })
```

### 3. LLM Prompt构建（推测）

服务器端可能使用的Prompt结构：
```
System Prompt:
你是一个智能助手，可以控制设备的各种功能。

可用的工具：
1. self.camera.take_photo
   描述：拍照并分析照片。当用户要求拍照、看东西、识别物体时使用。
   参数：
     - question: 关于照片的问题

2. self.audio_speaker.set_volume
   描述：设置音量
   参数：
     - volume: 0-100的整数

...

用户消息："拍一张照片"

请根据用户意图调用相应的工具。
```

---

## 设备端工具列表（发送给服务器）

### 获取工具列表

**文件：** `main/mcp_server.cc:257-305`

```cpp
void McpServer::GetToolsList(int id, const std::string& cursor) {
    const int max_payload_size = 8000;
    std::string json = "{\"tools\":[";
    
    // 遍历所有注册的工具
    for (auto it = tools_.begin(); it != tools_.end(); ++it) {
        std::string tool_json = (*it)->to_json() + ",";
        if (json.length() + tool_json.length() + 30 > max_payload_size) {
            // 如果超出大小限制，设置next_cursor
            next_cursor = (*it)->name();
            break;
        }
        json += tool_json;
    }
    
    json += "]}";
    ReplyResult(id, json);
}
```

**工具JSON格式（发送给服务器）：**
```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "tools": [
      {
        "name": "self.camera.take_photo",
        "description": "Take a photo and explain it. Use this tool after the user asks you to see something.\nArgs:\n  `question`: The question that you want to ask about the photo.\nReturn:\n  A JSON object that provides the photo information.",
        "inputSchema": {
          "type": "object",
          "properties": {
            "question": {
              "type": "string"
            }
          },
          "required": ["question"]
        }
      },
      {
        "name": "self.audio_speaker.set_volume",
        "description": "Set the volume of the audio speaker...",
        "inputSchema": {
          "type": "object",
          "properties": {
            "volume": {
              "type": "integer",
              "minimum": 0,
              "maximum": 100
            }
          },
          "required": ["volume"]
        }
      }
    ]
  }
}
```

**关键点：**
- 设备端在连接时或初始化时，服务器会调用 `tools/list` 获取可用工具
- 服务器根据工具描述构建LLM的system prompt
- LLM根据工具描述和用户消息，决定调用哪个工具

---

## 完整消息流示例

### 1. 设备发送音频
```
设备 → 服务器（WebSocket二进制帧）
[Opus编码的音频数据]
```

### 2. 服务器返回STT结果
```
服务器 → 设备（WebSocket JSON）
{
  "type": "stt",
  "text": "拍一张照片"
}
```

### 3. 服务器发送MCP工具调用
```
服务器 → 设备（WebSocket JSON）
{
  "type": "mcp",
  "payload": {
    "jsonrpc": "2.0",
    "method": "tools/call",
    "params": {
      "name": "self.camera.take_photo",
      "arguments": {
        "question": "用户想要拍照"
      }
    },
    "id": 1
  }
}
```

### 4. 设备执行工具并返回结果
```
设备 → 服务器（WebSocket JSON）
{
  "type": "mcp",
  "payload": {
    "jsonrpc": "2.0",
    "id": 1,
    "result": {
      "content": [
        {
          "type": "text",
          "text": "{\"success\": true, \"result\": \"照片已拍摄，画面中看到...\"}"
        }
      ],
      "isError": false
    }
  }
}
```

---

## 关键代码文件总结

### 设备端（ESP32）

| 功能 | 文件 | 关键函数 |
|------|------|----------|
| **音频采集** | `main/audio/audio_service.cc` | `AudioInputTask()` |
| **音频编码** | `main/audio/audio_service.cc` | Opus编码 |
| **发送音频** | `main/protocols/websocket_protocol.cc` | `SendAudio()` |
| **接收STT** | `main/application.cc:436-443` | `OnMessage()` 处理 `type: "stt"` |
| **接收MCP** | `main/application.cc:451-455` | `OnMessage()` 处理 `type: "mcp"` |
| **解析工具调用** | `main/mcp_server.cc:154-238` | `ParseMessage()` |
| **执行工具** | `main/mcp_server.cc:308-367` | `DoToolCall()` |
| **工具列表** | `main/mcp_server.cc:257-305` | `GetToolsList()` |

### 服务器端（不在设备代码中）

| 功能 | 推测实现 |
|------|----------|
| **STT语音识别** | 使用ASR服务（如Whisper、百度ASR等） |
| **LLM API调用** | 调用OpenAI、Claude、本地LLM等 |
| **Prompt构建** | 根据工具列表构建system prompt |
| **语义理解** | LLM根据用户消息和工具描述决定调用 |

---

## 为什么设备端没有关键词识别？

**设计原因：**
1. **灵活性**：使用LLM语义理解，可以理解各种表达方式
   - "拍一张照片"
   - "帮我拍个照"
   - "我想看看现在的情况"
   - "用摄像头拍一下"
   
   所有这些都能被LLM理解，而不需要硬编码关键词。

2. **可扩展性**：添加新功能只需要：
   - 在设备端注册新工具
   - 服务器自动获取工具列表
   - LLM自动学会使用新工具
   
   不需要修改关键词匹配逻辑。

3. **准确性**：LLM可以理解上下文和意图，比关键词匹配更准确。

---

## 总结

1. **设备端职责**：
   - ✅ 采集和发送音频
   - ✅ 接收STT文本（仅显示，不处理）
   - ✅ 接收MCP工具调用请求
   - ✅ 执行工具并返回结果
   - ❌ **不进行**语音识别
   - ❌ **不进行**语义理解
   - ❌ **不调用**LLM API

2. **服务器端职责**：
   - ✅ STT语音识别
   - ✅ LLM语义理解
   - ✅ Prompt构建
   - ✅ 工具调用决策

3. **通信协议**：
   - WebSocket（文本JSON + 二进制音频）
   - MCP协议（JSONRPC 2.0）

这种设计使得设备端代码简洁，所有复杂的AI处理都在服务器端完成，设备只需要执行命令即可。










