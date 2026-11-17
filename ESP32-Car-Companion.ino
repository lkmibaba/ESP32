// ============================================================
//  ESP32-Car-Companion 主程序
//  硬件: 微雪 ESP32-S3-Touch-AMOLED-1.75C
//  功能: 迎宾欢迎 + 驾驶监测 + 音乐频谱/时钟 + 分贝计
//  交互: 触屏点击切换界面
// ============================================================

#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "ui_screen.h"
#include "driving_monitor.h"
#include "audio_viz.h"
#include "welcome_detect.h"
#include <TouchDrvCST92xx.hpp>   // 官方触摸驱动 (CST9217, I2C 0x5A)

UiScreen        ui;
DrivingMonitor  driving;
AudioViz        audioViz;
WelcomeDetect   welcome;
TouchDrvCST92xx touch;   // 触摸控制器

ScreenMode currentScreen = SCREEN_WELCOME;

// 时钟字符串缓冲
char timeBuf[16];

// ---------- 提示音 (通过 ES8311 输出) ----------
void beep(int freq, int durationMs)
{
    audioViz.playTone(freq, durationMs);
}

// ---------- 获取时间字符串 ----------
void updateTimeString()
{
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 100)) {
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d",
                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    } else {
        snprintf(timeBuf, sizeof(timeBuf), "--:--:--");
    }
}

// ---------- 触摸切换界面 ----------
void handleTouch()
{
    static bool wasPressed = false;
    int16_t x, y;
    uint8_t touched = touch.getPoint(x, y, 20);
    bool nowPressed = (touched > 0);
    if (nowPressed && !wasPressed) {
        currentScreen = (ScreenMode)((currentScreen + 1) % SCREEN_COUNT);
    }
    wasPressed = nowPressed;
}

void setup()
{
    Serial.begin(115200);

    ui.begin();

    // 触摸控制器初始化 (I2C 0x5A, SDA=15 SCL=14)
    Wire.begin(IIC_SDA, IIC_SCL);
    pinMode(TP_RST, OUTPUT);
    digitalWrite(TP_RST, HIGH);
    delay(10);
    digitalWrite(TP_RST, LOW);
    delay(10);
    digitalWrite(TP_RST, HIGH);
    delay(100);
    if (!touch.begin(Wire, TOUCH_I2C_ADDR, TP_INT, TP_RST)) {
        Serial.println("Touch init failed!");
    }

#if ENABLE_WELCOME
    welcome.begin();
#endif

#if ENABLE_DRIVING
    driving.begin();
#endif

#if ENABLE_SPECTRUM || ENABLE_DECIBEL
    audioViz.begin();
#endif

    // 显示初始欢迎
    ui.drawWelcome("Connecting...");
}

void loop()
{
    handleTouch();

    switch (currentScreen) {
        case SCREEN_WELCOME:
        {
#if ENABLE_WELCOME
            static bool welcomed = false;
            if (!welcomed && welcome.detectAppleNearby()) {
                ui.drawWelcome("Hello!");
                beep(BEEP_WELCOME_HZ, 300);
                welcomed = true;
            } else if (welcomed) {
                // 保持在欢迎界面
            }
#else
            ui.drawWelcome("Car Companion");
#endif
            break;
        }

        case SCREEN_DRIVING:
        {
#if ENABLE_DRIVING
            DriveState st = driving.update();
            ui.drawDriving(driving.stateText(), driving.ax(), driving.ay(), driving.az());
            if (st == DRIVE_HARD_ACCEL || st == DRIVE_HARD_DECEL || st == DRIVE_TURNING) {
                beep(BEEP_ALERT_HZ, 200);
            }
#endif
            break;
        }

        case SCREEN_SPECTRUM:
        {
#if ENABLE_SPECTRUM
            static float bins[32];
            audioViz.updateSpectrum(bins, 32);
            updateTimeString();
            ui.drawSpectrum(bins, 32, timeBuf);
#endif
            break;
        }

        case SCREEN_DECIBEL:
        {
#if ENABLE_DECIBEL
            float db = audioViz.readDecibel();
            ui.drawDecibel(db);
#endif
            break;
        }
    }

    delay(50);
}
