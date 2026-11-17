#ifndef DRIVING_MONITOR_H
#define DRIVING_MONITOR_H

#include <Arduino.h>
#include "config.h"
#include "QMI8658.h"

// ============================================================
//  驾驶监测模块
//  通过 QMI8658 六轴 IMU 判断停车/移动/转弯/急加速/急减速
// ============================================================

enum DriveState {
    DRIVE_PARKED = 0,
    DRIVE_MOVING,
    DRIVE_TURNING,
    DRIVE_HARD_ACCEL,
    DRIVE_HARD_DECEL
};

class DrivingMonitor
{
public:
    bool begin()
    {
        // QMI8658 与触摸共用 I2C 总线 (SDA=15, SCL=14)
        Wire.begin(IIC_SDA, IIC_SCL);
        if (!_imu.begin()) {
            return false;
        }
        return true;
    }

    // 读取并更新状态
    DriveState update()
    {
        float ax, ay, az, gx, gy, gz;
        _imu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

        // 合加速度模长变化量 (去除重力, 简化: 用水平合加速度)
        float accelMag = sqrtf(ax * ax + ay * ay);
        float gyroMag = fabsf(gz);  // 主要看横摆角速度

        // 急加减速判断 (基于水平合加速度相对 1g 的偏差)
        float deltaAccel = accelMag - 1.0f;
        if (deltaAccel > ACCEL_EMERGENCY_ACC_THRESHOLD) {
            _state = DRIVE_HARD_ACCEL;
        } else if (deltaAccel < ACCEL_EMERGENCY_DEC_THRESHOLD) {
            _state = DRIVE_HARD_DECEL;
        } else if (gyroMag > GYRO_TURN_THRESHOLD) {
            _state = DRIVE_TURNING;
        } else if (accelMag > (1.0f + ACCEL_MOVING_THRESHOLD) ||
                   accelMag < (1.0f - ACCEL_MOVING_THRESHOLD)) {
            _state = DRIVE_MOVING;
        } else {
            _state = DRIVE_PARKED;
        }

        // 记录原始值供界面显示
        _ax = ax; _ay = ay; _az = az;
        return _state;
    }

    DriveState state() const { return _state; }
    float ax() const { return _ax; }
    float ay() const { return _ay; }
    float az() const { return _az; }

    // 状态转文字
    const char* stateText()
    {
        switch (_state) {
            case DRIVE_PARKED:     return "停车";
            case DRIVE_MOVING:     return "行驶中";
            case DRIVE_TURNING:    return "转弯";
            case DRIVE_HARD_ACCEL: return "急加速!";
            case DRIVE_HARD_DECEL: return "急减速!";
        }
        return "未知";
    }

private:
    QMI8658 _imu;
    DriveState _state = DRIVE_PARKED;
    float _ax = 0, _ay = 0, _az = 0;
};

#endif // DRIVING_MONITOR_H
