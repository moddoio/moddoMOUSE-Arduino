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
#define DPI_CHANGE_BUTTON_PIN 6

#ifndef ARRAY_SIZE
    // helpful macro
    #define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#endif

moddoMOUSE mouse;

uint16_t dpiOptions[] = {
    400,
    800,
    1600,
    3200,
};
uint8_t currentDpiIndex = 0;

// Use default of 1600 if not found
#define DEFAULT_DPI_INDEX 2

#if USE_LOW_POWER
void onWakeup()
{
    // Nothing to do in here
}
#endif

void setup()
{
    if (serialOutput) Serial.begin(9600);

    pinMode(DPI_CHANGE_BUTTON_PIN, INPUT_PULLUP);
#if USE_LOW_POWER
    LowPower.attachInterruptWakeup(DPI_CHANGE_BUTTON_PIN, onWakeup, FALLING);
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

    uint16_t x;
    uint16_t y;
    if (mouse.getDpiSettings(&x, &y) < 0) {
        if (serialOutput) Serial.println("Couldn't get DPI settings: error");
        return false;
    }

    if (serialOutput) {
        Serial.print("DPI X = ");
        Serial.println(x);
    }

    uint8_t i = 0;
    for(i = 0; i < ARRAY_SIZE(dpiOptions); i++) {
        if (dpiOptions[i] == x) {
            currentDpiIndex = i;
            break;
        }
    }
    if (i >= ARRAY_SIZE(dpiOptions)) {
        // Current value wasn't found in our options. Set to default
        currentDpiIndex = DEFAULT_DPI_INDEX;

        x = dpiOptions[currentDpiIndex];
        y = dpiOptions[currentDpiIndex];
        if (mouse.setDpiSettings(x, y) < 0) {
            if (serialOutput) Serial.println("Couldn't set DPI settings: error");
            return false;
        }
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

    bool pressed = !digitalRead(DPI_CHANGE_BUTTON_PIN);
    if (pressed) {
        while (!digitalRead(DPI_CHANGE_BUTTON_PIN)) {
            // wait for release
            delay(10);
        }

        currentDpiIndex++;
        currentDpiIndex %= ARRAY_SIZE(dpiOptions);

        uint16_t x = dpiOptions[currentDpiIndex];
        uint16_t y = dpiOptions[currentDpiIndex];

        if (serialOutput) {
            Serial.print("Changing DPI to ");
            Serial.println(x);
        }

        if (mouse.setDpiSettings(x, y) < 0) {
            if (serialOutput) Serial.println("Couldn't set DPI settings: error");
            mouseConnected = false;
            return;
        }

        // Blink currently selected index + 1
        for (uint8_t i = 0; i < currentDpiIndex + 1; i++) {
            digitalWrite(LED_BUILTIN, LED_STATE_ON);
            delay(250);
            digitalWrite(LED_BUILTIN, !LED_STATE_ON);
            delay(250);
        }
    }
#if USE_LOW_POWER
    // Sleep until button interrupt causes wakeup
    mouse.suspend();
    LowPower.sleep();
    mouse.resume();
#endif
}
