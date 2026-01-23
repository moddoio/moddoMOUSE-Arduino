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

#define DPI_CHANGE_BUTTON_PIN 3

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

void setup()
{
    Serial.begin(9600);

    pinMode(DPI_CHANGE_BUTTON_PIN, INPUT_PULLUP);

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
    if (mouse.readDeviceID(&deviceId) < 0) {
        Serial.println("Couldn't read device ID: error");
        return false;
    }

    Serial.print("Device ID = ");
    Serial.println(deviceId, HEX);

    uint16_t x;
    uint16_t y;
    if (mouse.getDpiSettings(&x, &y) < 0) {
        Serial.println("Couldn't get DPI settings: error");
        return false;
    }

    Serial.print("DPI X = ");
    Serial.println(x);

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
            Serial.println("Couldn't set DPI settings: error");
            return false;
        }
    }

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

        Serial.print("Changing DPI to ");
        Serial.println(x);

        if (mouse.setDpiSettings(x, y) < 0) {
            Serial.println("Couldn't set DPI settings: error");
            mouseConnected = false;
            return;
        }

        // Blink currently selected index + 1
        for (uint8_t i = 0; i < currentDpiIndex + 1; i++) {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(250);
            digitalWrite(LED_BUILTIN, LOW);
            delay(250);
        }
    }

}
