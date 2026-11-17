#ifndef WELCOME_DETECT_H
#define WELCOME_DETECT_H

#include <Arduino.h>
#include "config.h"
#include <NimBLEDevice.h>

// ============================================================
//  迎宾检测模块
//  通过 BLE 扫描检测附近 Apple 设备 (厂商 ID 0x004C),
//  配合 RSSI 阈值判断距离, 触发欢迎
// ============================================================

class WelcomeDetect
{
public:
    bool begin()
    {
        NimBLEDevice::init("ESP32-Car-Companion");
        _pScan = NimBLEDevice::getScan();
        _pScan->setActiveScan(true);
        _pScan->setInterval(80);
        _pScan->setWindow(40);
        return true;
    }

    // 扫描一次, 返回是否检测到 Apple 设备在阈值内
    bool detectAppleNearby()
    {
        unsigned long now = millis();
        if (now - _lastTrigger < WELCOME_COOLDOWN_MS) {
            return false; // 冷却期内
        }

        NimBLEScanResults results = _pScan->getResults(200); // 扫描 200ms
        bool found = false;
        for (int i = 0; i < results.getCount(); i++) {
            NimBLEAdvertisedDevice dev = results.getDevice(i);
            // 检查厂商数据是否为 Apple (Company ID 0x004C)
            if (isAppleDevice(dev) && dev.getRSSI() > WELCOME_RSSI_THRESHOLD) {
                found = true;
                break;
            }
        }
        _pScan->clearResults();

        if (found) {
            _lastTrigger = now;
        }
        return found;
    }

private:
    // 判断广播包是否来自 Apple 设备
    bool isAppleDevice(NimBLEAdvertisedDevice& dev)
    {
        std::string mfg = dev.getManufacturerData();
        if (mfg.size() < 2) return false;
        // 厂商 ID 为小端序, Apple = 0x004C -> 字节 4C 00
        uint16_t companyId = (uint8_t)mfg[0] | ((uint8_t)mfg[1] << 8);
        return companyId == WELCOME_APPLE_COMPANY_ID;
    }

    NimBLEScan* _pScan = nullptr;
    unsigned long _lastTrigger = 0;
};

#endif // WELCOME_DETECT_H
