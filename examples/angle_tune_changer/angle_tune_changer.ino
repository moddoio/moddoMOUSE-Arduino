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

#define ANGLE_INCREASE_BUTTON_PIN 3
#define ANGLE_DECREASE_BUTTON_PIN 4

#define ANGLE_STEP 5 // increase or decrease amount of degrees per button press

moddoMOUSE mouse;

int8_t currentAngleTune = 0;

void setup()
{
    Serial.begin(9600);

    pinMode(ANGLE_INCREASE_BUTTON_PIN, INPUT_PULLUP);
    pinMode(ANGLE_DECREASE_BUTTON_PIN, INPUT_PULLUP);

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

    if (mouse.getAngleTune(&currentAngleTune) < 0) {
        Serial.println("Couldn't get Angle Tune: error");
        return false;
    }

    Serial.print("Angle Tune = ");
    Serial.print(currentAngleTune);
    Serial.println(" °");

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

    bool inc = !digitalRead(ANGLE_INCREASE_BUTTON_PIN);
    bool dec = !digitalRead(ANGLE_DECREASE_BUTTON_PIN);
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

        Serial.print("Changing Angle Tune to ");
        Serial.println(currentAngleTune);

        if (mouse.setAngleTune(currentAngleTune) < 0) {
            Serial.println("Couldn't set Angle Tune: error");
            mouseConnected = false;
            return;
        }

        // Blink LED
        digitalWrite(LED_BUILTIN, HIGH);
        delay(250);
        digitalWrite(LED_BUILTIN, LOW);
        delay(250);
    }

}
