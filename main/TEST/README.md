# TEST 测试模块说明

## 重要说明：关于 main.cc 的调用

### ❌ 不需要在原来的 main.cc 里调用测试代码！

**原因：**

1. **测试模式下会替换 main.cc**
   - 当选择测试模式编译时，`main/CMakeLists.txt` 会**移除**原来的 `main.cc`
   - 然后**添加** `TEST/xxx_test/main.cc` 作为新的入口点
   - 这样就不会有 `app_main()` 冲突问题

2. **编译时选择**
   - 通过 `menuconfig` 或 `sdkconfig` 配置选择编译目标
   - 选择测试模式 → 编译测试程序
   - 不选择 → 编译正常的主程序

3. **完全隔离**
   - 测试代码和主程序代码完全隔离
   - 不会互相影响
   - 不需要修改原来的 `main.cc`

## 工作原理

### 正常模式（编译主程序）
```
main/
├── main.cc          ← 使用这个作为入口
└── TEST/            ← 不参与编译
```

### 测试模式（编译测试程序）
```
main/
├── main.cc          ← 被移除，不参与编译
└── TEST/
    └── screen_test/
        └── main.cc  ← 使用这个作为入口
```

## 如何编译和使用

### 1. 配置测试目标

在 `main/Kconfig.projbuild` 中添加配置选项（需要添加），然后：

```bash
idf.py menuconfig
```

在菜单中找到测试配置，选择要编译的测试目标。

### 2. 或者直接修改 sdkconfig

在 `sdkconfig` 文件中添加：
```
CONFIG_TEST_TARGET_SCREEN=y
# 或
CONFIG_TEST_TARGET_CAMERA=y
# 或
CONFIG_TEST_TARGET_VOICE=y
```

### 3. 编译和烧录

```bash
idf.py build
idf.py flash monitor
```

## 文件结构

```
main/TEST/
├── common/                    # 公共测试代码
│   ├── test_framework.h      # 测试框架接口
│   ├── test_framework.cpp    # 测试框架实现
│   ├── test_utils.h          # 工具函数接口
│   └── test_utils.cpp        # 工具函数实现
├── screen_test/              # 屏幕测试
│   ├── main.cc               # 测试入口（替换主程序的main.cc）
│   ├── screen_test.h         # 测试接口
│   └── screen_test.cpp       # 测试实现
├── camera_test/              # 摄像头测试
│   ├── main.cc
│   ├── camera_test.h
│   └── camera_test.cpp
└── voice_test/               # 语音测试
    ├── main.cc
    ├── voice_test.h
    └── voice_test.cpp
```

## 下一步

需要在 `main/CMakeLists.txt` 中添加测试模式的配置逻辑，让系统能够：
1. 检测是否选择了测试模式
2. 移除原来的 main.cc
3. 添加对应测试的 main.cc 和测试文件
4. 添加必要的包含路径






































