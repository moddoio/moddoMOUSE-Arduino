/*
 * moddoMOUSE Arduino Examples
 * Copyright (C) 2026 moddo inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * https://moddo.io
 * https://github.com/moddoio
 *
 */

#include <moddoMOUSE.h>

// Change this to 0 if you don't want low power support (easier when debugging).
// If you don't use it the Arduino power consumption will be ~200x higher at idle,
// and will drain the mouse battery.
#define USE_LOW_POWER_IF_SUPPORTED 1

#if USE_LOW_POWER_IF_SUPPORTED && defined(ARDUINO_ARCH_SAMD)
    // Low power library only supported on some Arduino platforms
    #include <ArduinoLowPower.h>
    #define USE_LOW_POWER 1
#else
    #define USE_LOW_POWER 0
#endif

#ifndef LED_STATE_ON
    #ifdef ARDUINO_SEEED_XIAO_M0
        #define LED_STATE_ON LOW
    #else
        #define LED_STATE_ON HIGH
    #endif
#endif


// Pin mappings
#define INTERRUPT_PIN 1

moddoMOUSE mouse;

#if USE_LOW_POWER
void onWakeup()
{
    // Nothing to do in here
}
#endif

void setup()
{
    Serial.begin(9600);

    // Using this LED to show when active
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LED_STATE_ON);

    pinMode(INTERRUPT_PIN, INPUT);
#if USE_LOW_POWER
    LowPower.attachInterruptWakeup(INTERRUPT_PIN, onWakeup, RISING);
#endif

    while (!Serial) {
        // blink LED to indicate waiting
        digitalWrite(LED_BUILTIN, !LED_STATE_ON);
        delay(200);
        digitalWrite(LED_BUILTIN, LED_STATE_ON);
    }
    Serial.println("setup done");
}

bool connect()
{
    if (mouse.begin() == 0) {
        Serial.println("Connected to moddoMOUSE");
    } else {
        Serial.println("Couldn't connect to moddoMOUSE");
        return false;
    }

    uint16_t deviceId;
    if (mouse.getDeviceID(&deviceId) < 0) {
        Serial.println("Couldn't read device ID: error");
        return false;
    }

    Serial.print("Device ID = ");
    Serial.println(deviceId, HEX);

    if (mouse.setBatteryChangeInterrupt(true) < 0) {
        Serial.println("Couldn't enable interrupt for battery changes");
        return false;
    }

    return true;
}

void loop()
{
    static bool mouseConnected = false;
    int ret;

    if (!mouseConnected) {
        mouseConnected = connect();
        if (!mouseConnected) {
            delay(100);
            return;
        }
    }

    // Read battery info
    struct batteryStatus status;
    ret = mouse.getBatteryStatus(&status);
    if (ret < 0) {
        Serial.println("Couldn't read battery status: error");
        mouseConnected = false;
        return;
    }

    if (!status.batteryPresent) {
        Serial.println("No battery");
    } else if (status.batteryCapacity == MODDOMOUSE_BAT_CAPACITY_UNKNOWN) {
        Serial.println("Battery capacity unknown");
    } else {
        // This is if USB is connected
        if (status.externalSupply) {
            Serial.println("USB connected");
        }

        Serial.print("Battery = ");
        Serial.print(status.batteryCapacity);
        Serial.print("%, (");
        Serial.print(status.batteryVoltage);
        Serial.println("mV)");

        if (status.batteryCharging) {
            Serial.println("Battery charging");
        } else if (status.externalSupply && status.batteryCapacity >= 90) {
            Serial.println("Battery full");
        } else {
            Serial.println("Battery discharging");
        }

        if (status.health != CHARGER_HEALTH_GOOD) {
            Serial.print("Charging error: ");
            Serial.println(status.health);
        }
    }
    Serial.flush();

#if USE_LOW_POWER
    // Sleep until interrupt from mouse causes wakeup
    if (digitalRead(INTERRUPT_PIN) == 0) {
        digitalWrite(LED_BUILTIN, !LED_STATE_ON);
        mouse.suspend();
        USBDevice.detach();
        LowPower.sleep();
        USBDevice.attach();
        mouse.resume();
        digitalWrite(LED_BUILTIN, LED_STATE_ON);

        // It appears the Serial is returned as 1 after the attach command
        // However it also appears to revert to 0 after 1 sec of inactivity
        // So we need to wait before the while check
        delay(1000);

        while (!Serial);
        Serial.println();
    }
#else
    // Wait for interrupt pin from moddoMOUSE
    while(digitalRead(INTERRUPT_PIN) == 0) {
    }
#endif
}
