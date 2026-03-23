# 关节关键词词表与回滚说明

## 目的

- 记录当前设备端本地关键词匹配规则（`application.cc`）。
- 明确关节 1 / 关节 5 的口令如何映射到动作。
- 提供回滚步骤，便于快速恢复到改动前行为。

## 当前本地关键词逻辑

设备在收到 STT 文本后，会先做本地关键词匹配（不依赖上位机语义），命中则直接执行关节动作。

### 关节标识词

- 关节 1：`关节1`、`关节一`、`j1`、`底座`
- 关节 5：`关节5`、`关节五`、`j5`、`灯头`、`头部`

### 方向词

- 左：`左转`、`向左`、`往左`、`左边`、`向左移动`、`左移`
- 右：`右转`、`向右`、`往右`、`右边`、`向右移动`、`右移`
- 上：`抬高`、`抬头`、`向上`、`往上`、`上抬`、`向上移动`、`上移`
- 下：`放低`、`低头`、`向下`、`往下`、`下压`、`抬低`、`向下移动`、`下移`

### 步进词（step_deg）

- 默认：`20`
- 小步：命中 `一点点`、`一点`、`稍微`、`轻轻`、`小一点` => `10`
- 大步：命中 `很大`、`大幅`、`大幅度` => `30`

### 速度词（speed_deg_per_s）

- 默认：关节 1 为 `15`，关节 5 为 `10`
- 慢速：命中 `慢一点`、`慢点`、`慢慢`、`缓慢`
  - 关节 1 => `8`
  - 关节 5 => `6`

## 执行映射

- 关节 1 左：`LampMoveJoint1Relative(-step_deg, speed_j1)`
- 关节 1 右：`LampMoveJoint1Relative(+step_deg, speed_j1)`
- 关节 5 上：`LampMoveJoint5Relative(+step_deg, speed_j5)`
- 关节 5 下：`LampMoveJoint5Relative(-step_deg, speed_j5)`

如果语句没有显式“关节1/关节5”标识：

- 命中左右方向词，优先按关节 1 处理
- 命中上下方向词，优先按关节 5 处理

## 维护建议

- 优先增加高置信短词，避免加入歧义词（如可能与聊天语义冲突的词）。
- 每次新增词后，做 4 条点对点测试：
  - `关节1向左移动`
  - `关节1向右移动`
  - `关节5向上移动`
  - `关节5向下移动`

## 回滚步骤

如需回滚到“仅上位机语义路由，不做本地关节关键词匹配”的状态：

1. 打开 `main/application.cc`。
2. 定位 `OnIncomingJson` 中 `type == "stt"` 的关键词处理段。
3. 删除或注释“Joint move keywords (local fallback path)”这一整段：
   - `mention_j1/mention_j5`
   - `left/right/up/down`
   - `step_deg/speed_j1/speed_j5`
   - `LampMoveJoint1Relative/LampMoveJoint5Relative` 调用
4. 保留原有开关灯关键词逻辑（`TurnOn/TurnOff`）。
5. 重新构建烧录：
   ```bash
   cd /home/zexuan/X/lab/xiaozhi-esp32
   source $HOME/esp/esp-idf/export.sh
   idf.py build
   idf.py -p /dev/ttyUSB0 flash
   ```

## 备注

- 本文仅记录关键词词表与回滚方式，不修改 `lamp_mcp_bridge` 的关节动作实现。
