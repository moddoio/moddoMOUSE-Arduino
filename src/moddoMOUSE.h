/*
 * moddoMOUSE Arduino Library
 * Copyright (C) 2026 moddo inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * https://moddo.io
 * https://github.com/moddoio
 *
 */

#ifndef MODDOMOUSE_H
#define MODDOMOUSE_H

#include "Arduino.h"
#include <Wire.h>

#define EIO 5
#define EINVAL 22

#define MODDOMOUSE_I2C_ADDRESS 0x0A

/**
 * @brief Charger health conditions
 *
 * These conditions determine the ability to, or the rate of, charge
 */
enum charger_health: uint8_t {
    /** Charger health condition is unknown */
    CHARGER_HEALTH_UNKNOWN = 0,
    /** Charger health condition is good */
    CHARGER_HEALTH_GOOD,
    /** The charger device is overheated */
    CHARGER_HEALTH_OVERHEAT,
    /** The battery voltage has exceeded its overvoltage threshold */
    CHARGER_HEALTH_OVERVOLTAGE,
    /**
     * The battery or charger device is experiencing an unspecified
     * failure.
     */
    CHARGER_HEALTH_UNSPEC_FAILURE,
    /** The battery temperature is below the "cold" threshold */
    CHARGER_HEALTH_COLD,
    /** The charger device's watchdog timer has expired */
    CHARGER_HEALTH_WATCHDOG_TIMER_EXPIRE,
    /** The charger device's safety timer has expired */
    CHARGER_HEALTH_SAFETY_TIMER_EXPIRE,
    /** The charger device requires calibration */
    CHARGER_HEALTH_CALIBRATION_REQUIRED,
    /** The battery temperature is in the "warm" range */
    CHARGER_HEALTH_WARM,
    /** The battery temperature is in the "cool" range */
    CHARGER_HEALTH_COOL,
    /** The battery temperature is below the "hot" threshold */
    CHARGER_HEALTH_HOT,
    /** The charger device does not detect a battery */
    CHARGER_HEALTH_NO_BATTERY,
};

#define MODDOMOUSE_BAT_CAPACITY_UNKNOWN 255

struct __attribute__((packed)) batteryStatus
{
    // Battery voltage in millivolts
    uint16_t batteryVoltage;

    // Battery capacity in percent (0-100), or MODDOMOUSE_BAT_CAPACITY_UNKNOWN
    uint8_t batteryCapacity;

    // true if a battery is connected
    uint8_t batteryPresent : 1;

    // true if battery is charging, false if discharging
    uint8_t batteryCharging : 1;

    // Will be one of the values in `enum charger_health`
    uint8_t health : 5;

    // true if USB supply is connected
    uint8_t externalSupply : 1;
};

#define DPI_MIN 50
#define DPI_MAX 26000

enum pollingRateOptions: uint8_t {
    POLLING_RATE_125,
    POLLING_RATE_250,
    POLLING_RATE_500,
    POLLING_RATE_1000,
};
#define POLLING_RATE_MAX POLLING_RATE_1000

enum liftDistanceOptions: uint8_t {
    LIFT_DISTANCE_1MM,
    LIFT_DISTANCE_2MM
};
#define LIFT_DISTANCE_MAX LIFT_DISTANCE_2MM

#define ANGLE_TUNE_MIN -30
#define ANGLE_TUNE_MAX 30

class moddoMOUSE
{
    public:
        moddoMOUSE();

        int8_t begin(uint8_t address = MODDOMOUSE_I2C_ADDRESS, TwoWire& wirePort = Wire);

        int8_t readProductID(uint8_t *value);
        int8_t readDeviceID(uint16_t *value);

        int8_t readBatteryStatus(struct batteryStatus *status);

        int8_t readMotion(int16_t *x, int16_t *y);
        int8_t readMainButtons(bool *left, bool *right, bool *middle, bool *back, bool *forward);

        int8_t writeButtons(uint16_t *button_bits);

        int8_t setBatteryChangeInterrupt(bool enable);
        int8_t setMotionInterrupt(bool enable);
        int8_t setMainButtonsInterrupt(bool enable);

        int8_t disableAllInterrupts();

        // Settings
        int8_t setDpiSettings(uint16_t x, uint16_t y);
        int8_t getDpiSettings(uint16_t *x, uint16_t *y);
        int8_t setInvertSwapSettings(bool invert_x, bool invert_y, bool swap_xy);
        int8_t getInvertSwapSettings(bool *invert_x, bool *invert_y, bool *swap_xy);
        int8_t setPollingRate(enum pollingRateOptions pollingRate);
        int8_t getPollingRate(enum pollingRateOptions *pollingRate);
        int8_t setLiftDistance(enum liftDistanceOptions liftDistance);
        int8_t getLiftDistance(enum liftDistanceOptions *liftDistance);
        int8_t setAngleTune(int8_t angle);
        int8_t getAngleTune(int8_t *angle);

        // Scroll wheels
        int8_t setVerticalWheel(int8_t delta);
        int8_t getVerticalWheel(int8_t *delta);
        int8_t setHorizontalWheel(int8_t delta);
        int8_t getHorizontalWheel(int8_t *delta);

    private:
        int i2cRead(uint8_t reg, void *buf, uint8_t length);
        int i2cWrite(uint8_t reg, void *buf, uint8_t length);

        uint8_t i2cAddress;
        TwoWire* i2cPort;

        bool mouseConnected;
};

#endif // MODDOMOUSE_H