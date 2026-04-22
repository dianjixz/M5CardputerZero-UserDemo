# M5CardputerZero-UserDemo

[English](./README.md)

基于 [M5Stack_Linux_Libs](https://github.com/m5stack/M5Stack_Linux_Libs) SDK 开发的 M5Cardputer Zero 用户演示程序。该项目展示了如何在 M5Cardputer Zero（AArch64 Linux）设备上使用 **LVGL 9.5** 构建图形界面应用。

UI 界面由 **SquareLine Studio 1.5.0** 生成，包含设备状态栏（Logo、时钟、电量）和主页面内容区域，支持 SDL2 仿真模式（本机调试）和 Linux Framebuffer 模式（设备运行）两种显示后端。

---

## 项目结构

```
M5CardputerZero-UserDemo/
├── SDK/                        # M5Stack_Linux_Libs SDK（git submodule）
│   ├── components/             # 组件库（lvgl_component、DeviceDriver 等）
│   ├── examples/               # SDK 示例程序
│   └── tools/                  # 编译工具链脚本（SCons）
├── projects/
│   └── UserDemo/               # 用户演示项目
│       ├── SConstruct          # 项目顶层编译脚本
│       ├── config_defaults.mk  # 默认编译配置（启用 LVGL 9.5 等）
│       └── main/               # 主程序源码
│           ├── SConstruct      # 组件编译脚本
│           ├── src/
│           │   └── main.cpp    # 程序入口（LVGL 初始化、显示后端选择）
│           ├── include/
│           │   └── main.h      # 键盘队列等公共声明
│           └── ui/             # SquareLine Studio 生成的 UI 代码
│               ├── ui.h / ui.c # UI 初始化
│               ├── screens/    # 屏幕定义（Screen1）
│               ├── components/ # 自定义组件
│               ├── fonts/      # 字体资源
│               └── images/     # 图片资源（Logo、时钟图标、电量图标）
└── README.md
```

---

## 功能特性

- 基于 LVGL 9.5 的图形界面
- 状态栏显示：M5Stack Zero Logo、时钟时间、电量百分比
- 主内容区：应用名称 + 页面内容占位
- 支持两种显示后端：
  - **SDL2**：用于 PC 端仿真调试（默认编译模式）
  - **Linux Framebuffer（ST7789V）**：用于 M5Cardputer Zero 设备端运行
- 支持 evdev 键盘/触摸输入（设备端）
- 使用 SCons + Kconfig 构建系统，可通过 `scons menuconfig` 灵活配置

---

## 环境准备

### 依赖安装（仅需执行一次）

```bash
sudo apt update
sudo apt install python3 python3-pip libffi-dev libsdl2-dev

pip3 install parse scons requests tqdm
pip3 install setuptools-rust paramiko scp
```

> Python 版本需 ≥ 3.8

### 克隆项目（含子模块）

```bash
git clone --recursive https://github.com/dianjixz/M5CardputerZero-UserDemo.git
cd M5CardputerZero-UserDemo
```

如果已克隆但未初始化子模块：

```bash
git submodule update --init --recursive
```

---

## 编译

### 方式一：SDL2 仿真模式（PC 本机调试，默认）

`projects/UserDemo/SConstruct` 中默认启用 SDL2 后端，直接编译即可在 PC 上运行：

```bash
cd projects/UserDemo
scons -j$(nproc)
```

编译产物位于 `projects/UserDemo/build/` 目录下，可执行文件名为 `M5CardputerZero-UserDemo`。

### 方式二：交叉编译（AArch64，部署到 M5Cardputer Zero 设备）

需要安装 AArch64 交叉编译工具链：

```bash
# 方法1：通过 apt 安装
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

```

修改 `projects/UserDemo/SConstruct`，将 SDL2 后端切换为 Framebuffer 后端：

```python
# 将以下代码块的 if True 改为 if False（禁用 SDL2）：
if False:   # <-- 改为 False
    if not os.path.exists('build/config/config_tmp.mk'):
        ...
        f.write('CONFIG_V9_5_LV_USE_SDL=y\n')

# 将以下代码块的 if False 改为 if True（启用 Framebuffer）：
if True:    # <-- 改为 True
    if not os.path.exists('build/config/config_tmp.mk'):
        ...
        f.write('CONFIG_V9_5_LV_USE_LINUX_FBDEV=y\n')
        f.write('CONFIG_V9_5_LV_DRAW_SW_ASM_NEON=y\n')
        f.write('CONFIG_TOOLCHAIN_PREFIX="aarch64-linux-gnu-"\n')
```

然后编译：

```bash
cd projects/UserDemo
scons -j$(nproc)
```

### 配置管理命令

```bash
# 查看/修改编译配置（图形化菜单）
scons menuconfig

# 清理编译产物
scons -c

# 彻底清理（含配置缓存）
scons distclean

# 详细编译输出
scons verbose
```

---

## 运行

### PC 端（SDL2 仿真）

```bash
cd projects/UserDemo
./dist/M5CardputerZero-UserDemo
```

### M5Cardputer Zero 设备端

将编译产物推送到设备：

```bash
scp -r projects/UserDemo/dist user@<device_ip>:/home/user/
```

在设备上运行（使用 Framebuffer）：

```bash
# 自动检测 ST7789V Framebuffer 设备
./dist/M5CardputerZero-UserDemo

# 或手动指定 Framebuffer 设备
export LV_LINUX_FBDEV_DEVICE=/dev/fb0
./dist/M5CardputerZero-UserDemo

# 指定键盘输入设备
export LV_LINUX_KEYBOARD_DEVICE=/dev/input/by-path/platform-3f804000.i2c-event
./dist/M5CardputerZero-UserDemo
```

---

## UI 界面说明

界面由 SquareLine Studio 1.5.0 设计生成，分辨率为 **320×170**（ST7789V 屏幕）。

| 区域 | 内容 |
|------|------|
| 左上角 | M5Stack Zero Logo 图标 |
| 右上角（时钟） | 时钟图标 + 当前时间标签 |
| 右上角（电量） | 电量图标 + 电量百分比标签 |
| 顶部中央 | 应用名称（APPName） |
| 主内容区 | 应用页面内容（320×143 区域） |

如需修改 UI，可用 SquareLine Studio 打开项目后重新生成 `projects/UserDemo/main/ui/` 目录下的代码。

---

## 相关资源

- [M5Stack_Linux_Libs SDK](https://github.com/m5stack/M5Stack_Linux_Libs)
- [LVGL 文档](https://docs.lvgl.io/)
- [SquareLine Studio](https://squareline.io/)
- [M5Cardputer Zero 产品页](https://docs.m5stack.com/)
