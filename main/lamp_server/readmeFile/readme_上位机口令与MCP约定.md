# 上位机口令与 MCP 约定（历史说明）

## 说明

- 该文档保留为历史兼容说明。
- 当前建议优先使用 `readme_关节1和5专用工具隔离与回滚.md` 中的 4 个专用工具通道。

## 历史工具（可能已下线）

- `self.servo.rotate`
- `self.lamp.set_angles`
- `self.lamp.joint5.relative_move`

## 当前推荐（专用隔离）

- `self.lamp.j1.left_step`
- `self.lamp.j1.right_step`
- `self.lamp.j5.up_step`
- `self.lamp.j5.down_step`
