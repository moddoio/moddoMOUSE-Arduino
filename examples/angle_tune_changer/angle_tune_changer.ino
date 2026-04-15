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

// Don't output on serial port in low power mode
bool serialOutput = !USE_LOW_POWER;


// Pin mappings. Make sure to use pins that support interrupts if using low power sleep
#define ANGLE_INCREASE_BUTTON_PIN 6
#define ANGLE_DECREASE_BUTTON_PIN 7

#define ANGLE_STEP 5 // increase or decrease amount of degrees per button press

moddoMOUSE mouse;

int8_t currentAngleTune = 0;

volatile bool incPressed = false;
volatile bool decPressed = false;

#if USE_LOW_POWER
void incInt()
{
    incPressed = true;
}

void decInt()
{
    decPressed = true;
}
#endif

void setup()
{
    if (serialOutput) Serial.begin(9600);

    pinMode(ANGLE_INCREASE_BUTTON_PIN, INPUT_PULLUP);
    pinMode(ANGLE_DECREASE_BUTTON_PIN, INPUT_PULLUP);
#if USE_LOW_POWER
    LowPower.attachInterruptWakeup(ANGLE_INCREASE_BUTTON_PIN, incInt, FALLING);
    LowPower.attachInterruptWakeup(ANGLE_DECREASE_BUTTON_PIN, decInt, FALLING);
#endif

    if (serialOutput) {
        Serial.println("setup done");
        Serial.flush();
        delay(2000);
    }
}

bool connect()
{
    if (mouse.begin() == 0) {
        if (serialOutput) Serial.println("Connected to moddoMOUSE");
    } else {
        if (serialOutput) Serial.println("Couldn't connect to moddoMOUSE");
        return false;
    }

    uint16_t deviceId;
    if (mouse.getDeviceID(&deviceId) < 0) {
        if (serialOutput) Serial.println("Couldn't read device ID: error");
        return false;
    }

    if (serialOutput) {
        Serial.print("Device ID = ");
        Serial.println(deviceId, HEX);
    }

    if (mouse.getAngleTune(&currentAngleTune) < 0) {
        if (serialOutput) Serial.println("Couldn't get Angle Tune: error");
        return false;
    }

    if (serialOutput) {
        Serial.print("Angle Tune = ");
        Serial.print(currentAngleTune);
        Serial.println(" °");
    }

#if USE_LOW_POWER
    // Disable USB once we're connected to mouse to save power
    // Don't disable any earlier to make programming easier (when disconnected from mouse)
    USBDevice.detach();
    USBDevice.end();
#endif

    return true;
}

void loop()
{
    static bool mouseConnected = false;

    if (!mouseConnected) {
        mouseConnected = connect();
        if (!mouseConnected) {
            delay(100);
            return;
        }
    }

#if USE_LOW_POWER
    noInterrupts(); // we need to do these operations atomically
    bool inc = incPressed;
    incPressed = false;
    bool dec = decPressed;
    decPressed = false;
    interrupts();
#else
    bool inc = !digitalRead(ANGLE_INCREASE_BUTTON_PIN);
    bool dec = !digitalRead(ANGLE_DECREASE_BUTTON_PIN);
#endif
    if (inc || dec) {
        int8_t direction = inc ? 1 : -1;
        while (inc || dec) {
            // wait for release
            delay(10);
            inc = !digitalRead(ANGLE_INCREASE_BUTTON_PIN);
            dec = !digitalRead(ANGLE_DECREASE_BUTTON_PIN);
        }

        currentAngleTune += (ANGLE_STEP * direction);

        if (currentAngleTune < ANGLE_TUNE_MIN) {
            currentAngleTune = ANGLE_TUNE_MIN;
        }
        if (currentAngleTune > ANGLE_TUNE_MAX) {
            currentAngleTune = ANGLE_TUNE_MAX;
        }

        if (serialOutput) {
            Serial.print("Changing Angle Tune to ");
            Serial.println(currentAngleTune);
        }

        if (mouse.setAngleTune(currentAngleTune) < 0) {
            if (serialOutput) Serial.println("Couldn't set Angle Tune: error");
            mouseConnected = false;
            return;
        }

        // Blink LED
        digitalWrite(LED_BUILTIN, LED_STATE_ON);
        delay(250);
        digitalWrite(LED_BUILTIN, !LED_STATE_ON);
        delay(250);
    } else {
#if USE_LOW_POWER
        // Sleep until button interrupt causes wakeup
        mouse.suspend();
        LowPower.sleep();
        mouse.resume();
#endif
    }
}
