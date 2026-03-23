# 关节1和关节5专用工具隔离与回滚说明

## 本次修改目的

- 将关节1（左右）和关节5（上下）的成功逻辑做成独立 MCP 工具通道。
- 避免语音口令被误路由到旧的 `self.servo.rotate` 或 `self.lamp.set_angles`。
- 只改了 `main/mcp_server.cc` 的工具注册层，未改 `lamp_mcp_bridge` 的关节1/5执行逻辑。

## 当前启用的专用工具

- `self.lamp.j1.left_step`：关节1向左相对步进
- `self.lamp.j1.right_step`：关节1向右相对步进
- `self.lamp.j5.up_step`：关节5向上相对步进
- `self.lamp.j5.down_step`：关节5向下相对步进

参数约定：

- `step_deg`：步进角度，默认 10，范围 1~30
- `speed_deg_per_s`：速度，j1 默认 15（范围 1~90），j5 默认 10（范围 1~60）

## 本次下线（未删除）工具

在 `main/mcp_server.cc` 中仅注释说明为 disabled，未删代码主体：

- `self.servo.rotate`
- `self.lamp.set_angles`
- `self.lamp.joint5.relative_move`（旧名称）

## 回滚到本次修改前的方法

1. 打开 `main/mcp_server.cc`。
2. 删除（或注释掉）四个专用工具：
   - `self.lamp.j1.left_step`
   - `self.lamp.j1.right_step`
   - `self.lamp.j5.up_step`
   - `self.lamp.j5.down_step`
3. 恢复旧工具注册块：
   - `self.servo.rotate`
   - `self.lamp.set_angles`
   - `self.lamp.joint5.relative_move`
4. 保存后重新构建并烧录：
   ```bash
   cd /home/zexuan/X/lab/xiaozhi-esp32
   source $HOME/esp/esp-idf/export.sh
   idf.py build
   idf.py -p /dev/ttyUSB0 flash
   ```

## 备注

- 这份改动专门用于“关节1和关节5与其他逻辑隔离”。
- 若后续要继续做“新机制语义层（mechanism/rules）”，建议在独立分支推进，避免再次与本隔离方案混用。
