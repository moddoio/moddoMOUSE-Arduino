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

// Pin mappings
#define INTERRUPT_PIN 1

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

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, !LED_STATE_ON);

    pinMode(INTERRUPT_PIN, INPUT);
#if USE_LOW_POWER
    LowPower.attachInterruptWakeup(INTERRUPT_PIN, onWakeup, RISING);
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

    if (mouse.setMainButtonsInterrupt(true) < 0) {
        if (serialOutput) Serial.println("Couldn't enable interrupt for buttons");
        return false;
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
    ret = mouse.getMainButtons(&left, &right, &middle, &back, &forward);
    if (ret < 0) {
        if (serialOutput) Serial.println("Couldn't read buttons: error");
        mouseConnected = false;
        return;
    }

    if (serialOutput) {
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
    }

    if (left || right || middle || back || forward) {
        // Show LED if button pressed
        digitalWrite(LED_BUILTIN, LED_STATE_ON);
    } else {
        digitalWrite(LED_BUILTIN, !LED_STATE_ON);
    }

#if USE_LOW_POWER
    // Sleep until interrupt from mouse causes wakeup
    if (digitalRead(INTERRUPT_PIN) == 0) {
        mouse.suspend();
        LowPower.sleep();
        mouse.resume();
    }
#else
    // Wait for interrupt pin from moddoMOUSE
    while(digitalRead(INTERRUPT_PIN) == 0) {
    }
#endif
}
