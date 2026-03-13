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

#define BUTTON_PIN_START 3
#define BUTTON_PIN_END   12
#define NUM_BUTTONS      (BUTTON_PIN_END - BUTTON_PIN_START + 1)

moddoMOUSE mouse;

void setup()
{
    Serial.begin(9600);

    for (uint8_t pin = BUTTON_PIN_START; pin <= BUTTON_PIN_END; pin++) {
        pinMode(pin, INPUT_PULLUP);
    }

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
    return true;
}

void loop()
{
    static uint16_t lastButtonBits = 0;
    static bool mouseConnected = false;
    int ret;

    if (!mouseConnected) {
        mouseConnected = connect();
        if (!mouseConnected) {
            delay(100);
            return;
        }
    }

    // Read button and send to mouse if changed
    int buttons[NUM_BUTTONS];
    for (uint8_t pin = BUTTON_PIN_START; pin <= BUTTON_PIN_END; pin++) {
        buttons[pin - BUTTON_PIN_START] = !digitalRead(pin);
    }
    uint16_t buttonBits = 0;
    for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
        buttonBits |= (buttons[i] & 1) << i;
    }
    if (buttonBits != lastButtonBits) {
        lastButtonBits = buttonBits;

        ret = mouse.writeButtons(&buttonBits);
        if (ret < 0) {
            Serial.println("Couldn't write buttons: error");
            mouseConnected = false;
            return;
        }

        Serial.print("Wrote button register = ");
        Serial.println(buttonBits, HEX);
    }
}
