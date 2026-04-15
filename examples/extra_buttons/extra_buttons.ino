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
#define BUTTON_PIN_START 4
#define BUTTON_PIN_END   9
#define NUM_BUTTONS      (BUTTON_PIN_END - BUTTON_PIN_START + 1)

moddoMOUSE mouse;

volatile bool buttonChange = false;

#if USE_LOW_POWER
void onWakeup()
{
    buttonChange = true;
}
#endif

void setup()
{
    if (serialOutput) Serial.begin(9600);

    for (uint8_t pin = BUTTON_PIN_START; pin <= BUTTON_PIN_END; pin++) {
        pinMode(pin, INPUT_PULLUP);
#if USE_LOW_POWER
        LowPower.attachInterruptWakeup(pin, onWakeup, CHANGE);
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
    static uint32_t lastButtonBits = 0;
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
    uint32_t buttonBits = 0;
    for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
        buttonBits |= (buttons[i] & 1) << i;
    }
    if (buttonBits != lastButtonBits) {
        lastButtonBits = buttonBits;

        ret = mouse.setButtons(buttonBits);
        if (ret < 0) {
            if (serialOutput) Serial.println("Couldn't write buttons: error");
            mouseConnected = false;
            return;
        }

        if (serialOutput) {
            Serial.print("Wrote button register = ");
            Serial.println(buttonBits, HEX);
        }
    }
#if USE_LOW_POWER
    noInterrupts(); // we need to do this operation atomically
    bool change = buttonChange;
    buttonChange = false;
    interrupts();
s
    if (!change) {
        // Sleep until button interrupt causes wakeup.
        // NOTE: There might be extra latency to your button handling caused by coming out of sleep,
        // so you might only want to sleep after a period of inactivity.
        mouse.suspend();
        LowPower.sleep();
        mouse.resume();

        // Clear interrupt flag, since we're about to handle it
        buttonChange = false;
    } else {
        // Button was changed while were handling previous one, don't sleep yet
    }
#endif
}
