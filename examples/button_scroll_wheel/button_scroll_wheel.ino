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

#define V_WHEEL_UP_BUTTON_PIN 3
#define V_WHEEL_DOWN_BUTTON_PIN 4
#define H_WHEEL_UP_BUTTON_PIN 5
#define H_WHEEL_DOWN_BUTTON_PIN 6

moddoMOUSE mouse;

void setup()
{
    Serial.begin(9600);

    pinMode(V_WHEEL_UP_BUTTON_PIN, INPUT_PULLUP);
    pinMode(V_WHEEL_DOWN_BUTTON_PIN, INPUT_PULLUP);
    pinMode(H_WHEEL_UP_BUTTON_PIN, INPUT_PULLUP);
    pinMode(H_WHEEL_DOWN_BUTTON_PIN, INPUT_PULLUP);

    Serial.println("setup done");
    Serial.flush();
    delay(2000);
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
            Serial.print("Changing V wheel by ");
            Serial.println(vDirection);

            if (mouse.setVerticalWheel(vDirection) < 0) {
                Serial.println("Couldn't set V wheel: error");
                mouseConnected = false;
                return;
            }
        }

        if (hDirection != 0) {
            Serial.print("Changing H wheel by ");
            Serial.println(hDirection);

            if (mouse.setHorizontalWheel(hDirection) < 0) {
                Serial.println("Couldn't set V wheel: error");
                mouseConnected = false;
                return;
            }
        }
    }

}
