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

#include <moddoMOUSE.h>

#define INTERRUPT_PIN 2

moddoMOUSE mouse;

void setup()
{
    Serial.begin(9600);
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

    if (mouse.setMotionInterrupt(true) < 0) {
        Serial.println("Couldn't enable interrupt for motion");
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

    // Read X, Y deltas
    int16_t x;
    int16_t y;
    ret = mouse.readMotion(&x, &y);
    if (ret < 0) {
        Serial.println("Couldn't read motion: error");
        mouseConnected = false;
        return;
    }

    if (x != 0 || y != 0) {
#if 1
        // These prints slow down the updating of x/y values due to the serial output speed
        Serial.print("X = ");
        Serial.print(x);
        Serial.print(", Y = ");
        Serial.println(y);
#else
        // Alternatively, just show LED if motion delta is high enough
        // The resulting acceleration here depends on the speed of the Arduino,
        // so a less trivial algorithm should be used in practice.
        if (abs(x) > 10 || abs(y) > 10) {
            digitalWrite(LED_BUILTIN, HIGH);
        } else {
            digitalWrite(LED_BUILTIN, LOW);
        }
#endif
    } else {
        // Wait for interrupt pin from moddoMOUSE
        while(digitalRead(INTERRUPT_PIN) == 0) {
            delay(1);
        }
    }
}
