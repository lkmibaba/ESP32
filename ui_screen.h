#ifndef UI_SCREEN_H
#define UI_SCREEN_H

#include <Arduino_GFX_Library.h>
#include "config.h"

// ============================================================
//  UI 渲染模块
//  使用 Arduino_GFX (官方 GFX_Library_for_Arduino) 驱动 CO5300
//  QSPI 接口, 1.75" 圆形 AMOLED, 466x466
//  引脚已按微雪官方 pin_config.h 核实
// ============================================================

// 界面枚举
enum ScreenMode {
    SCREEN_WELCOME = 0,
    SCREEN_DRIVING,
    SCREEN_SPECTRUM,
    SCREEN_DECIBEL,
    SCREEN_COUNT
};

class UiScreen
{
public:
    void begin()
    {
        // CO5300 通过 QSPI 总线连接
        Arduino_DataBus* bus = new Arduino_ESP32QSPI(
            LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);
        _gfx = new Arduino_CO5300(
            bus, LCD_RESET, 0 /* rotation */,
            LCD_WIDTH, LCD_HEIGHT, 6 /* offset */, 0, 0, 0);

        if (!_gfx->begin()) {
            Serial.println("gfx->begin() failed!");
        }
        _gfx->fillScreen(RGB565_BLACK);
    }

    // 绘制欢迎界面
    void drawWelcome(const char* message)
    {
        _gfx->fillScreen(RGB565_BLACK);
        _gfx->setTextColor(RGB565_WHITE);
        _gfx->setTextSize(3);
        int16_t w = _gfx->width();
        _gfx->setCursor((w - 7 * 3 * 6) / 2, SCREEN_HEIGHT / 2 - 80);
        _gfx->print("Welcome");

        _gfx->setTextColor(RGB565_GREEN);
        _gfx->setTextSize(2);
        _gfx->setCursor(w / 2 - strlen(message) * 12, SCREEN_HEIGHT / 2 + 20);
        _gfx->print(message);
    }

    // 绘制驾驶监测界面
    void drawDriving(const char* state, float accelX, float accelY, float accelZ)
    {
        _gfx->fillScreen(RGB565_BLACK);
        _gfx->setTextColor(RGB565_WHITE);
        _gfx->setTextSize(2);
        int16_t w = _gfx->width();
        _gfx->setCursor(w / 2 - 7 * 12, 60);
        _gfx->print("Driving");

        // 状态
        _gfx->setTextColor(RGB565_YELLOW);
        _gfx->setTextSize(3);
        _gfx->setCursor(w / 2 - strlen(state) * 18, 150);
        _gfx->print(state);

        // 加速度
        char buf[48];
        _gfx->setTextColor(RGB565_WHITE);
        _gfx->setTextSize(2);
        snprintf(buf, sizeof(buf), "ax:%.2fg ay:%.2fg", accelX, accelY);
        _gfx->setCursor(w / 2 - strlen(buf) * 12, 240);
        _gfx->print(buf);
        snprintf(buf, sizeof(buf), "az:%.2fg", accelZ);
        _gfx->setCursor(w / 2 - strlen(buf) * 12, 290);
        _gfx->print(buf);
    }

    // 绘制频谱 + 时钟界面
    void drawSpectrum(float* bins, int binCount, const char* timeStr)
    {
        _gfx->fillScreen(RGB565_BLACK);
        // 时钟
        _gfx->setTextColor(RGB565_WHITE);
        _gfx->setTextSize(2);
        int16_t w = _gfx->width();
        _gfx->setCursor(w / 2 - strlen(timeStr) * 12, 40);
        _gfx->print(timeStr);

        // 频谱柱
        int barW = (w - 40) / binCount;
        int baseY = SCREEN_HEIGHT - 30;
        int maxH = SCREEN_HEIGHT - 110;
        for (int i = 0; i < binCount; i++) {
            int h = (int)(bins[i] * maxH);
            if (h < 2) h = 2;
            if (h > maxH) h = maxH;
            int x = 20 + i * barW;
            _gfx->fillRect(x, baseY - h, barW - 2, h, RGB565_GREEN);
        }
    }

    // 绘制分贝计界面
    void drawDecibel(float db)
    {
        _gfx->fillScreen(RGB565_BLACK);
        _gfx->setTextColor(RGB565_WHITE);
        _gfx->setTextSize(2);
        int16_t w = _gfx->width();
        _gfx->setCursor(w / 2 - 24, 120);
        _gfx->print("dB");

        char buf[24];
        snprintf(buf, sizeof(buf), "%.1f", db);
        _gfx->setTextColor(db > 80 ? RGB565_RED : RGB565_GREEN);
        _gfx->setTextSize(4);
        _gfx->setCursor(w / 2 - strlen(buf) * 24, SCREEN_HEIGHT / 2 - 20);
        _gfx->print(buf);

        // 量程条
        int barY = SCREEN_HEIGHT - 60;
        int barW = w - 80;
        _gfx->drawRect(40, barY, barW, 16, RGB565_WHITE);
        int fillW = (int)((db / 120.0f) * barW);
        if (fillW > barW) fillW = barW;
        if (fillW < 0) fillW = 0;
        _gfx->fillRect(40, barY, fillW, 16, RGB565_GREEN);
    }

    Arduino_GFX* gfx() { return _gfx; }

private:
    Arduino_GFX* _gfx = nullptr;
};

#endif // UI_SCREEN_H
