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

// Don't output on serial port in low power mode
bool serialOutput = !USE_LOW_POWER;


// Pin mappings. Make sure to use pins that support interrupts if using low power sleep
#define V_WHEEL_UP_BUTTON_PIN 6
#define V_WHEEL_DOWN_BUTTON_PIN 7
#define H_WHEEL_UP_BUTTON_PIN 8
#define H_WHEEL_DOWN_BUTTON_PIN 9

moddoMOUSE mouse;

#if USE_LOW_POWER
void onWakeup()
{
    // Nothing to do in here
}
#endif

void setup()
{
    if (serialOutput) Serial.begin(9600);

    for (int pin = V_WHEEL_UP_BUTTON_PIN; pin <= H_WHEEL_DOWN_BUTTON_PIN; pin++) {
        pinMode(pin, INPUT_PULLUP);
#if USE_LOW_POWER
        LowPower.attachInterruptWakeup(pin, onWakeup, FALLING);
#endif
    }

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

    int8_t vDirection = 0;
    bool vUp = !digitalRead(V_WHEEL_UP_BUTTON_PIN);
    bool vDown = !digitalRead(V_WHEEL_DOWN_BUTTON_PIN);
    if (vUp || vDown) {
        vDirection = vUp ? 1 : -1;
    }
    int8_t hDirection = 0;
    bool hUp = !digitalRead(H_WHEEL_UP_BUTTON_PIN);
    bool hDown = !digitalRead(H_WHEEL_DOWN_BUTTON_PIN);
    if (hUp || hDown) {
        hDirection = hUp ? 1 : -1;
    }

    if (vDirection != 0 || hDirection != 0) {
        bool buttonPressed = true;
        while (buttonPressed) {
            // wait for release
            delay(10);

            // re-read buttons
            buttonPressed = false;
            if(!digitalRead(V_WHEEL_UP_BUTTON_PIN)) {
                buttonPressed = true;
            } else if(!digitalRead(V_WHEEL_DOWN_BUTTON_PIN)) {
                buttonPressed = true;
            } else if(!digitalRead(H_WHEEL_UP_BUTTON_PIN)) {
                buttonPressed = true;
            } else if(!digitalRead(H_WHEEL_DOWN_BUTTON_PIN)) {
                buttonPressed = true;
            }
        }

        if (vDirection != 0) {
            if (serialOutput) {
                Serial.print("Changing V wheel by ");
                Serial.println(vDirection);
            }

            if (mouse.setVerticalWheel(vDirection) < 0) {
                if (serialOutput) Serial.println("Couldn't set V wheel: error");
                mouseConnected = false;
                return;
            }
        }

        if (hDirection != 0) {
            if (serialOutput) {
                Serial.print("Changing H wheel by ");
                Serial.println(hDirection);
            }

            if (mouse.setHorizontalWheel(hDirection) < 0) {
                if (serialOutput) Serial.println("Couldn't set V wheel: error");
                mouseConnected = false;
                return;
            }
        }
    }
#if USE_LOW_POWER
    // Sleep until button interrupt causes wakeup
    mouse.suspend();
    LowPower.sleep();
    mouse.resume();
#endif
}
