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
    if (mouse.getDeviceID(&deviceId) < 0) {
        Serial.println("Couldn't read device ID: error");
        return false;
    }

    Serial.print("Device ID = ");
    Serial.println(deviceId, HEX);

    if (mouse.setBatteryChangeInterrupt(true) < 0) {
        Serial.println("Couldn't enable interrupt for battery changes");
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

    // Read battery info
    struct batteryStatus status;
    ret = mouse.getBatteryStatus(&status);
    if (ret < 0) {
        Serial.println("Couldn't read battery status: error");
        mouseConnected = false;
        return;
    }

    if (!status.batteryPresent) {
        Serial.println("No battery");
    } else if (status.batteryCapacity == MODDOMOUSE_BAT_CAPACITY_UNKNOWN) {
        Serial.println("Battery capacity unknown");
    } else {
        // This is if USB is connected
        if (status.externalSupply) {
            Serial.println("USB connected");
        }

        Serial.print("Battery = ");
        Serial.print(status.batteryCapacity);
        Serial.print("%, (");
        Serial.print(status.batteryVoltage);
        Serial.println("mV)");

        if (status.batteryCharging) {
            Serial.println("Battery charging");
        } else if (status.externalSupply && status.batteryCapacity >= 90) {
            Serial.println("Battery full");
        } else {
            Serial.println("Battery discharging");
        }

        if (status.health != CHARGER_HEALTH_GOOD) {
            Serial.print("Charging error: ");
            Serial.println(status.health);
        }
    }
    Serial.flush();

    // Ideally, would put the processor to sleep here and use a real interrupt instead of looping
    while(digitalRead(INTERRUPT_PIN) == 0) {
        delay(100);
    }
}
