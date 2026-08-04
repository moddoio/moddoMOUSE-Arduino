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

// configure deep sleep setting on mouse. set to 0 to disable deep sleep timeout
#define DEEP_SLEEP_TIMEOUT_MINUTES 10

// Pin mappings

// interrupt from moddoMOUSE
#define INTERRUPT_PIN 1

// pin attached to button to enter deep sleep early
#define DEEP_SLEEP_PIN 6

// pin attached to button to enter power off
#define POWER_OFF_PIN 7

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
    pinMode(DEEP_SLEEP_PIN, INPUT_PULLUP);
    pinMode(POWER_OFF_PIN, INPUT_PULLUP);
#if USE_LOW_POWER
    LowPower.attachInterruptWakeup(INTERRUPT_PIN, onWakeup, RISING);
    LowPower.attachInterruptWakeup(DEEP_SLEEP_PIN, onWakeup, FALLING);
    LowPower.attachInterruptWakeup(POWER_OFF_PIN, onWakeup, FALLING);
#endif

    while (!Serial) {
        // blink LED to indicate waiting
        digitalWrite(LED_BUILTIN, !LED_STATE_ON);
        delay(200);
        digitalWrite(LED_BUILTIN, LED_STATE_ON);
    }
    Serial.println();
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

    if (mouse.setStatusInterrupt(true) < 0) {
        Serial.println("Couldn't enable interrupt for status changes");
        return false;
    }

    if (mouse.setDeepSleepTimeout(DEEP_SLEEP_TIMEOUT_MINUTES) < 0) {
        Serial.println("Couldn't set deep sleep timeout");
        return false;
    }

    return true;
}

void print_mode(enum mouseMode mode) {
    switch(mode) {
        case MODE_RUNNING:
            Serial.println("mode = Running");
            break;
        case MODE_IDLE:
            Serial.println("mode = Idle");
            break;
        case MODE_DEEP_SLEEP:
            Serial.println("mode = Deep Sleep");
            break;
        case MODE_POWER_OFF:
            Serial.println("mode = Power Off");
            break;
        default:
            break;
    }
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

    // Read mouse status
    struct mouseStatus status;
    ret = mouse.getStatus(&status);
    if (ret < 0) {
        Serial.println("Couldn't read mouse status: error");
        mouseConnected = false;
        return;
    }
    print_mode((enum mouseMode)status.mode);

    enum mouseMode modeToEnter = MODE_RUNNING;

    if (digitalRead(DEEP_SLEEP_PIN) == 0) {
        Serial.println("Deep sleep button pressed");
        modeToEnter = MODE_DEEP_SLEEP;
    } else if (digitalRead(POWER_OFF_PIN) == 0) {
        Serial.println("Power off button pressed");
        modeToEnter = MODE_POWER_OFF;
    }

    if (modeToEnter != MODE_RUNNING) {
        // enter mode
        status.mode = modeToEnter;
        ret = mouse.setStatus(&status);
        if (ret < 0) {
            Serial.println("Couldn't set mouse status: error");
            mouseConnected = false;
            return;
        }

        // let the mouse process the request
        delay(1);

        // check that the change took effect
        ret = mouse.getStatus(&status);
        if (ret < 0) {
            Serial.println("Couldn't read mouse status: error");
            mouseConnected = false;
            return;
        }

        if (status.mode != modeToEnter) {
            Serial.println("Mouse still active. Mode change will be delayed until conditions are met");
            // For deep sleep, mouse needs to be idle first.
            // For power off, mouse needs to be disconnected from USB first.
        }
        print_mode((enum mouseMode)status.mode);
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
    // Wait for interrupt pin from moddoMOUSE or button
    while(digitalRead(INTERRUPT_PIN) == 0 && digitalRead(DEEP_SLEEP_PIN) == 1) {
    }
#endif
}
