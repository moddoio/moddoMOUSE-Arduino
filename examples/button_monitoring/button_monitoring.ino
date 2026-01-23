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

    if (mouse.setMainButtonsInterrupt(true) < 0) {
        Serial.println("Couldn't enable interrupt for buttons");
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

    // Read main buttons
    bool left;
    bool right;
    bool middle;
    bool back;
    bool forward;
    ret = mouse.readMainButtons(&left, &right, &middle, &back, &forward);
    if (ret < 0) {
        Serial.println("Couldn't read motion: error");
        mouseConnected = false;
        return;
    }

    Serial.print("Left = ");
    Serial.print(left);
    Serial.print(", Right = ");
    Serial.print(right);
    Serial.print(", Mid = ");
    Serial.print(middle);
    Serial.print(", Back = ");
    Serial.print(back);
    Serial.print(", Forward = ");
    Serial.println(forward);

    // Wait for interrupt pin from moddoMOUSE
    while(digitalRead(INTERRUPT_PIN) == 0) {
        delay(1);
    }
}
