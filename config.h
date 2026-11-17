#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
//  ESP32-Car-Companion 配置头文件
//  硬件: 微雪 ESP32-S3-Touch-AMOLED-1.75C
//  引脚定义依据微雪官方 board_config/pin_config.h 核实
// ============================================================

// ---------- 功能开关 ----------
#define ENABLE_WELCOME      1   // 迎宾检测 (BLE)
#define ENABLE_DRIVING      1   // 驾驶监测 (IMU)
#define ENABLE_SPECTRUM     1   // 音乐频谱 + 时钟
#define ENABLE_DECIBEL      1   // 分贝计

// ============================================================
//  引脚定义 (微雪官方 pin_config.h)
// ============================================================

// ---- 屏幕 CO5300 (QSPI 接口) ----
#define LCD_SDIO0   4
#define LCD_SDIO1   5
#define LCD_SDIO2   6
#define LCD_SDIO3   7
#define LCD_SCLK    38
#define LCD_RESET   1
#define LCD_CS      12
#define LCD_WIDTH   466
#define LCD_HEIGHT  466

// ---- 触摸 CST9217 (I2C 地址 0x5A) ----
#define IIC_SDA     15
#define IIC_SCL     14
#define TP_INT      11
#define TP_RST      2
#define TOUCH_I2C_ADDR  0x5A

// ---- 音频 ES7210 (ADC, 采麦克风) / ES8311 (DAC, 播放) ----
#define PIN_ES7210_BCLK   9
#define PIN_ES7210_LRCK   45
#define PIN_ES7210_DIN    10
#define PIN_ES7210_MCLK   16
#define PIN_ES8311_DOUT   8
#define PIN_PA            46   // 功放使能 (PA)

// ---------- 屏幕尺寸 ----------
#define SCREEN_WIDTH        LCD_WIDTH
#define SCREEN_HEIGHT       LCD_HEIGHT

// ---------- 迎宾检测 (BLE) ----------
#define WELCOME_APPLE_COMPANY_ID    0x004C   // Apple 厂商 ID (BLE)
#define WELCOME_RSSI_THRESHOLD      -60      // RSSI 阈值 (dBm), 越大越近
#define WELCOME_COOLDOWN_MS         30000    // 触发冷却时间, 避免反复触发 (30s)

// ---------- 驾驶监测阈值 (IMU) ----------
// QMI8658 输出单位为 g (加速度) 与 dps (角速度)
#define ACCEL_EMERGENCY_ACC_THRESHOLD   0.8f   // 急加速阈值 (g)
#define ACCEL_EMERGENCY_DEC_THRESHOLD  -0.8f   // 急减速阈值 (g)
#define GYRO_TURN_THRESHOLD             80.0f  // 转弯判定角速度 (deg/s)
#define ACCEL_MOVING_THRESHOLD          0.08f  // 移动判定 (g, 加速度变化量)

// ---------- 分贝计 ----------
#define DB_REF_SAMPLE_RATE      16000     // 采样率 (Hz)
#define DB_FFT_SIZE             256       // FFT 点数

// ---------- 提示音 (通过 ES8311 播放) ----------
#define BEEP_WELCOME_HZ     2093          // 欢迎: C7 高音
#define BEEP_ALERT_HZ       880           // 告警: A5

#endif // CONFIG_H
