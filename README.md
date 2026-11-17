# ESP32-Car-Companion 车载多功能设备

基于微雪 **ESP32-S3-Touch-AMOLED-1.75C** 的车载多功能设备，纯板载、零外接。

## 功能

| 界面 | 功能 | 说明 |
|---|---|---|
| 迎宾欢迎 | BLE 扫描检测 Apple 设备 | 检测到附近 Apple 设备 → 屏显欢迎 + 提示音 |
| 驾驶监测 | 六轴 IMU | 停车/移动/转弯/急加速/急减速，屏幕 + 提示音提醒 |
| 频谱时钟 | 双麦 FFT + 时钟 | 音乐频谱柱动画 + 实时时钟 |
| 分贝计 | 双麦声压级 | 实时分贝数字 + 量程条 |

**交互**：触屏点击切换界面。

## 引脚定义（已核实，来自微雪官方 pin_config.h）

| 外设 | 引脚 |
|---|---|
| 屏幕 CO5300 (QSPI) | SDIO0-3 = GPIO 4/5/6/7，SCLK=38，CS=12，RST=1 |
| 触摸 CST9217 | I2C 0x5A，SDA=15，SCL=14，INT=11，RST=2 |
| 音频 ES7210 (采麦) | BCLK=9，LRCK=45，DIN=10，MCLK=16 |
| 音频 ES8311 (播放) | DOUT=8，功放 PA=46 |
| IMU QMI8658 | I2C（与触摸共用）SDA=15，SCL=14 |

## 依赖库（来自官方示例压缩包，需离线/手动安装）

从微雪官方仓库下载源码包，把 `examples/arduino/libraries` 下的库复制到 Arduino 库目录：

```
https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C
```

需要安装以下库：

| 库 | 用途 |
|---|---|
| `GFX_Library_for_Arduino` | CO5300 屏幕驱动（Arduino_GFX） |
| `SensorLib` | CST9217 触摸 + QMI8658 IMU 驱动 |
| `XPowersLib` | AXP2101 电源管理 |
| `board_config` | 引脚宏定义（已内联到本项目 config.h） |

## 开发环境

1. Arduino IDE 安装 ESP32 板包（esp32 by Espressif，选 **ESP32S3 Dev Module**，PSRAM 选 OPI，Flash 32MB）
2. 安装上述依赖库
3. 打开 `ESP32-Car-Companion.ino`，编译烧录

## 已完成（全部代码已就绪，含音频 codec 驱动集成）

- 屏幕 CO5300 QSPI 引脚与初始化（`ui_screen.h`）
- 触摸 CST9217 引脚与 I2C 地址（主程序 setup）
- IMU QMI8658 引脚（`driving_monitor.h`）
- 音频 ES7210/ES8311 采集+播放（`audio_viz.h`，含 FFT 频谱、分贝、提示音）
- 驾驶监测状态机、BLE 迎宾、界面渲染逻辑

## ⚠️ 唯一需要你手动操作的一步：复制 codec 驱动源文件

`audio_viz.h` 依赖微雪官方的 ES7210 / ES8311 驱动源码，**这 6 个文件已自动下载到本项目目录**（`es7210.h/.cpp`、`audio_hal.h`、`es8311.h/.c/.reg.h`），与 `.ino` 文件并列，Arduino 会自动一起编译，无需再手动复制。

> 如需自行核对，来源为官方仓库 `examples/arduino/examples/` 下的 `06_ES7210/` 和 `07_ES8311/` 目录。
> 注：`06_ES7210` 示例的 VAD 语音检测库本项目**未使用**，故未引入。

## 额外依赖

- `arduinoFFT`（FFT 频谱，通过 Arduino 库管理器安装）

## 默认阈值（可在 config.h 调整）

- 急加速：> +0.8g
- 急减速：< -0.8g
- 转弯：角速度 > 80°/s
- 迎宾 RSSI：> -60 dBm

## 使用说明

1. 上电默认进入欢迎界面，持续扫描 Apple 设备。
2. 靠近 Apple Watch 等 Apple 设备（约 1 米内），触发欢迎提示。
3. 点击屏幕切换「欢迎 → 驾驶监测 → 频谱时钟 → 分贝计」。
