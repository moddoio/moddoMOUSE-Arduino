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

// Change this to 0 if you don't want low power support
#define USE_LOW_POWER_IF_SUPPORTED 1

#if USE_LOW_POWER_IF_SUPPORTED && defined(ARDUINO_ARCH_SAMD)
// Low power library only supported on some Arduino platforms
#include <ArduinoLowPower.h>
#define USE_LOW_POWER 1
#else
#define USE_LOW_POWER 0
#endif

#define INTERRUPT_PIN 1

// Don't output on serial port in low power mode
bool serialOutput = !USE_LOW_POWER;

bool printMotion = true;

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
    if (mouse.readDeviceID(&deviceId) < 0) {
        if (serialOutput) Serial.println("Couldn't read device ID: error");
        return false;
    }

    if (serialOutput) {
        Serial.print("Device ID = ");
        Serial.println(deviceId, HEX);
    }

    if (mouse.setMotionInterrupt(true) < 0) {
        if (serialOutput) Serial.println("Couldn't enable interrupt for motion");
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
        if (serialOutput) Serial.println("Couldn't read motion: error");
        mouseConnected = false;
        return;
    }

    if (x != 0 || y != 0) {
        if (serialOutput && printMotion) {
            // These prints slow down the updating of x/y values due to the serial output speed
            Serial.print("X = ");
            Serial.print(x);
            Serial.print(", Y = ");
            Serial.println(y);
        }
        // Alternatively, just show LED during motion
        digitalWrite(LED_BUILTIN, HIGH);

        // Show for long enough to be seen
        delay(1);
    } else {
        digitalWrite(LED_BUILTIN, LOW);
#if USE_LOW_POWER
        // Sleep until button interrupt causes wakeup
        if (digitalRead(INTERRUPT_PIN) == 0) {
            LowPower.sleep();
        }
#else
        // Wait for interrupt pin from moddoMOUSE
        while(digitalRead(INTERRUPT_PIN) == 0) {
        }
#endif
    }
}
