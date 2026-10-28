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
    #if defined(ARDUINO_MODDO_PINCH)
        #include <PinchLowPower.h>
    #else
        #include <ArduinoLowPower.h>
    #endif
    #define USE_LOW_POWER 1
#else
    #define USE_LOW_POWER 0
#endif

#ifndef ARRAY_SIZE
    // helpful macro
    #define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#endif

// Don't output on serial port in low power mode
bool serialOutput = !USE_LOW_POWER;


// Pin mappings. Make sure to use pins that support interrupts if using low power sleep
#define BUTTON_PIN 11

// Joystick is powered by GPIO output connected to its +5V pin (it doesn't need to be 5V - it's just a potentiometer).
// This is so we can disable it when it's not being read and save power.
#define JOYSTICK_POWER_PIN 2

// Bit 10 is Left joystick down in button map
#define BUTTON_BIT_POSITION 10

#define ANALOG_Y_PIN A0
#define ANALOG_X_PIN A1

#define AVG_BUF_SIZE 8

moddoMOUSE mouse;

#if USE_LOW_POWER
void onWakeup()
{
    // Nothing to do in here
}

void onTick() {
    // Nothing to do in here
}
#endif

void setup()
{
    if (serialOutput) Serial.begin(9600);

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(JOYSTICK_POWER_PIN, OUTPUT);
    digitalWrite(JOYSTICK_POWER_PIN, 0);

#if USE_LOW_POWER
    LowPower.attachInterruptWakeup(BUTTON_PIN, onWakeup, FALLING);
    // 128Hz is the fastest wakeup frequency supported by the RTC
    // Requires v0.5.0 of moddo SAMD boards package
    LowPower.attachInterruptWakeup(RTC_PERIODIC_WAKEUP, onTick, PinchRTC::PER_128HZ);
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

#if USE_LOW_POWER && !defined(USB_DISABLED)
    // Disable USB once we're connected to mouse to save power
    // Don't disable any earlier to make programming easier (when disconnected from mouse)
    USBDevice.detach();
    USBDevice.end();
#endif

    return true;
}

int8_t readAnalogAxisLevel(int pin)
{
    return map(analogRead(pin), 0, 1023, JOYSTICK_MIN_VAL, JOYSTICK_MAX_VAL);
}

void loop()
{
    static int8_t avgXBuf[AVG_BUF_SIZE] = {0};
    static int8_t avgYBuf[AVG_BUF_SIZE] = {0};
    static int avgIdx = 0;
    static int8_t lastX = 0;
    static int8_t lastY = 0;
    static bool mouseConnected = false;

    if (!mouseConnected) {
        mouseConnected = connect();
        if (!mouseConnected) {
            delay(100);
            return;
        }
    }

    digitalWrite(JOYSTICK_POWER_PIN, 1);

    int8_t x = readAnalogAxisLevel(ANALOG_X_PIN);
    int8_t y = readAnalogAxisLevel(ANALOG_Y_PIN);

    digitalWrite(JOYSTICK_POWER_PIN, 0);

    int button = !digitalRead(BUTTON_PIN);
    uint16_t button_bits = (button & 1) << BUTTON_BIT_POSITION;

    mouse.setGamepadButtons(button_bits);

    // The analog reads can be noisy and bounce between two values,
    // so do some basic smoothing here.
    avgXBuf[avgIdx] = x;
    avgYBuf[avgIdx] = y;
    avgIdx++;
    avgIdx %= AVG_BUF_SIZE;

    int32_t xAvg = 0;
    int32_t yAvg = 0;
    for (int i = 0; i < AVG_BUF_SIZE; i++) {
        xAvg += avgXBuf[i];
        yAvg += avgYBuf[i];
    }
    xAvg /= AVG_BUF_SIZE;
    yAvg /= AVG_BUF_SIZE;

    if (serialOutput) {
        if (xAvg != lastX) {
            Serial.print("xAvg = ");
            Serial.println(xAvg);
            lastX = xAvg;
        }
        if (yAvg != lastY) {
            Serial.print("yAvg = ");
            Serial.println(yAvg);
            lastY = yAvg;
        }
    }

    mouse.setGamepadJoystick(0, xAvg, yAvg);

#if USE_LOW_POWER
    mouse.suspend();
    // Sleep until RTC tick or button press
    LowPower.sleep();
    mouse.resume();
#else
    delay(10);
#endif
}
