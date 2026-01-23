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

#define POLLLING_RATE_BUTTON_PIN 3
#define LIFT_DISTANCE_BUTTON_PIN 4
#define INVERT_X_BUTTON_PIN 5
#define INVERT_Y_BUTTON_PIN 6
#define SWAP_XY_BUTTON_PIN 7

#ifndef ARRAY_SIZE
    // helpful macro
    #define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#endif

moddoMOUSE mouse;

enum pollingRateOptions currentPollingRate;
enum liftDistanceOptions currentLiftDistance;
bool currentInvertX;
bool currentInvertY;
bool currentSwapXY;

void setup()
{
    Serial.begin(9600);

    for(uint8_t pin = POLLLING_RATE_BUTTON_PIN; pin <= SWAP_XY_BUTTON_PIN; pin++) {
        pinMode(pin, INPUT_PULLUP);
    }

    Serial.println("setup done");
    Serial.flush();
    delay(2000);
}

void printPollingRate()
{
    Serial.print("Polling rate = ");
    switch(currentPollingRate) {
    case POLLING_RATE_125:
        Serial.println("125");
        break;
    case POLLING_RATE_250:
        Serial.println("250");
        break;
    case POLLING_RATE_500:
        Serial.println("500");
        break;
    case POLLING_RATE_1000:
        Serial.println("1000");
        break;
    }
}

void printInvertSwap()
{
    Serial.print("Invert X = ");
    Serial.print(currentInvertX);
    Serial.print(", Invert Y = ");
    Serial.print(currentInvertY);
    Serial.print(", Swap XY = ");
    Serial.println(currentSwapXY);
}

void printLiftDistance()
{
    Serial.print("Lift distance = ");
    switch(currentLiftDistance) {
    case LIFT_DISTANCE_1MM:
        Serial.println("1mm");
        break;
    case LIFT_DISTANCE_2MM:
        Serial.println("2mm");
        break;
    }
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

    if (mouse.getPollingRate(&currentPollingRate) < 0) {
        Serial.println("Couldn't get polling rate: error");
        return false;
    }
    printPollingRate();


    if (mouse.getInvertSwapSettings(&currentInvertX, &currentInvertY, &currentSwapXY) < 0) {
        Serial.println("Couldn't get invert swap settings: error");
        return false;
    }
    printInvertSwap();

    if (mouse.getLiftDistance(&currentLiftDistance) < 0) {
        Serial.println("Couldn't get lift distance: error");
        return false;
    }
    printLiftDistance();

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

    for(uint8_t pin = POLLLING_RATE_BUTTON_PIN; pin <= SWAP_XY_BUTTON_PIN; pin++) {
        bool pressed = !digitalRead(pin);
        if (pressed) {
            while (!digitalRead(pin)) {
                // wait for release
                delay(10);
            }

            int ret;
            switch(pin) {
            case POLLLING_RATE_BUTTON_PIN:
                switch(currentPollingRate) {
                case POLLING_RATE_125:
                    currentPollingRate = POLLING_RATE_250;
                    break;
                case POLLING_RATE_250:
                    currentPollingRate = POLLING_RATE_500;
                    break;
                case POLLING_RATE_500:
                    currentPollingRate = POLLING_RATE_1000;
                    break;
                case POLLING_RATE_1000:
                    currentPollingRate = POLLING_RATE_125;
                    break;
                }
                ret = mouse.setPollingRate(currentPollingRate);
                printPollingRate();
                break;
            case LIFT_DISTANCE_BUTTON_PIN:
                switch(currentLiftDistance) {
                case LIFT_DISTANCE_1MM:
                    currentLiftDistance = LIFT_DISTANCE_2MM;
                    break;
                case LIFT_DISTANCE_2MM:
                    currentLiftDistance = LIFT_DISTANCE_1MM;
                    break;
                }
                ret = mouse.setLiftDistance(currentLiftDistance);
                printLiftDistance();
                break;
            case INVERT_X_BUTTON_PIN:
                currentInvertX = !currentInvertX;
                ret = mouse.setInvertSwapSettings(currentInvertX, currentInvertY, currentSwapXY);
                printInvertSwap();
                break;
            case INVERT_Y_BUTTON_PIN:
                currentInvertY = !currentInvertY;
                ret = mouse.setInvertSwapSettings(currentInvertX, currentInvertY, currentSwapXY);
                printInvertSwap();
                break;
            case SWAP_XY_BUTTON_PIN:
                currentSwapXY = !currentSwapXY;
                ret = mouse.setInvertSwapSettings(currentInvertX, currentInvertY, currentSwapXY);
                printInvertSwap();
                break;
            default:
                Serial.print("Invalid pin ");
                Serial.println(pin);
                break;
            }
            if (ret < 0) {
                Serial.println("Couldn't change settings: error");
                mouseConnected = false;
                return;
            }

            // Blink once
            digitalWrite(LED_BUILTIN, HIGH);
            delay(250);
            digitalWrite(LED_BUILTIN, LOW);
        }
    }
}
