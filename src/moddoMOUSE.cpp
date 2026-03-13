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

#include "moddoMOUSE.h"
#include "moddomouseRegs.h"

moddoMOUSE::moddoMOUSE()
{
}

int moddoMOUSE::i2cRead(uint8_t reg, void *buf, uint8_t length)
{
    uint8_t i = 0;
    uint8_t *bytePtr = (uint8_t *)buf;

    i2cPort->beginTransmission(i2cAddress);
    i2cPort->write(reg);
    int ret = i2cPort->endTransmission(false);
    if (ret != 0) {
        // I2C error
        return -EIO;
    }

    i2cPort->requestFrom((uint8_t)i2cAddress, length, (uint8_t)true);
    while ((i2cPort->available()) && (i < length)) {
        *bytePtr = i2cPort->read();
        bytePtr++;
        i++;
    }
    return i;
}

int moddoMOUSE::i2cWrite(uint8_t reg, void *buf, uint8_t length)
{
    uint8_t i = 0;
    uint8_t *bytePtr = (uint8_t *)buf;

    i2cPort->beginTransmission(i2cAddress);
    i2cPort->write(reg);
    while (i < length) {
        i2cPort->write(*bytePtr);
        bytePtr++;
        i++;
    }
    int ret = i2cPort->endTransmission(true);
    if (ret != 0) {
        // I2C error
        return -EIO;
    }

    return i;
}

// Read moddoMOUSE product ID
int8_t moddoMOUSE::readProductID(uint8_t *value)
{
    if (value == NULL) {
        return -EINVAL;
    }

    int8_t bytesRead = i2cRead(REG_PROD_ID, value, sizeof(*value));
    if (bytesRead != sizeof(*value)) {
        return -EIO;
    }

    return 0;
}

// Read moddoMOUSE device ID
int8_t moddoMOUSE::readDeviceID(uint16_t *value)
{
    if (value == NULL) {
        return -EINVAL;
    }

    int8_t bytesRead = i2cRead(REG_DEVICE_ID, value, sizeof(*value));
    if (bytesRead != sizeof(*value)) {
        return -EIO;
    }

    return 0;
}

// Read moddoMOUSE battery capacity and charger status
int8_t moddoMOUSE::readBatteryStatus(struct batteryStatus *status)
{
    if (status == NULL) {
        return -EINVAL;
    }

    int8_t bytesRead = i2cRead(REG_BATTERY_VOLTAGE, status, sizeof(*status));
    if (bytesRead != sizeof(*status)) {
        return -EIO;
    }

    return 0;
}

// Read moddoMOUSE motion
int8_t moddoMOUSE::readMotion(int16_t *x, int16_t *y)
{
    uint16_t motion[2];

    int8_t bytesRead = i2cRead(REG_MOUSE_X, motion, sizeof(motion));
    if (bytesRead != sizeof(motion)) {
        return -EIO;
    }

    if (x != NULL) {
        *x = motion[0];
    }
    if (y != NULL) {
        *y = motion[1];
    }

    return 0;
}

int8_t moddoMOUSE::readMainButtons(bool *left, bool *right, bool *middle, bool *back, bool *forward)
{
    uint8_t buttons;

    int8_t bytesRead = i2cRead(REG_MAIN_BUTTONS, &buttons, sizeof(buttons));
    if (bytesRead != sizeof(buttons)) {
        return -EIO;
    }

    if (left != NULL) {
        *left = (buttons & REG_MAIN_BUTTONS_LEFT_CLICK);
    }
    if (right != NULL) {
        *right = (buttons & REG_MAIN_BUTTONS_RIGHT_CLICK);
    }
    if (middle != NULL) {
        *middle = (buttons & REG_MAIN_BUTTONS_MIDDLE_CLICK);
    }
    if (back != NULL) {
        *back = (buttons & REG_MAIN_BUTTONS_BACK_CLICK);
    }
    if (forward != NULL) {
        *forward = (buttons & REG_MAIN_BUTTONS_FORWARD_CLICK);
    }

    return 0;
}

/**
 * @brief Write expansion board button states
 *
 * Each bit (out of 16) is a different button. Set the bit (1) when button is down, clear it (0) when up.
 *
 */
int8_t moddoMOUSE::writeButtons(uint16_t *button_bits)
{
    if (button_bits == NULL) {
        return -EINVAL;
    }

    int8_t bytesWritten = i2cWrite(REG_EXP_BOARD_BUTTONS, button_bits, sizeof(*button_bits));
    if (bytesWritten != sizeof(*button_bits)) {
        return -EIO;
    }

    return 0;
}

int8_t moddoMOUSE::setBatteryChangeInterrupt(bool enable)
{
    uint8_t value;

    int8_t bytesRead = i2cRead(REG_INT_EN, &value, sizeof(value));
    if (bytesRead != sizeof(value)) {
        return -EIO;
    }

    if (enable) {
        value |= REG_REG_INT_EN_BATTERY_CHANGE;
    } else {
        value &= ~REG_REG_INT_EN_BATTERY_CHANGE;
    }

    int8_t bytesWritten = i2cWrite(REG_INT_EN, &value, sizeof(value));
    if (bytesWritten != sizeof(value)) {
        return -EIO;
    }

    return 0;
}

int8_t moddoMOUSE::setMotionInterrupt(bool enable)
{
    uint8_t value;

    int8_t bytesRead = i2cRead(REG_INT_EN, &value, sizeof(value));
    if (bytesRead != sizeof(value)) {
        return -EIO;
    }

    if (enable) {
        value |= REG_REG_INT_EN_MOTION;
    } else {
        value &= ~REG_REG_INT_EN_MOTION;
    }

    int8_t bytesWritten = i2cWrite(REG_INT_EN, &value, sizeof(value));
    if (bytesWritten != sizeof(value)) {
        return -EIO;
    }

    return 0;
}

int8_t moddoMOUSE::setMainButtonsInterrupt(bool enable)
{
    uint8_t value;

    int8_t bytesRead = i2cRead(REG_INT_EN, &value, sizeof(value));
    if (bytesRead != sizeof(value)) {
        return -EIO;
    }

    if (enable) {
        value |= REG_REG_INT_EN_MAIN_BUTTONS;
    } else {
        value &= ~REG_REG_INT_EN_MAIN_BUTTONS;
    }

    int8_t bytesWritten = i2cWrite(REG_INT_EN, &value, sizeof(value));
    if (bytesWritten != sizeof(value)) {
        return -EIO;
    }

    return 0;
}

int8_t moddoMOUSE::disableAllInterrupts()
{
    uint8_t value = 0;

    // Disable interrupts
    int8_t bytesWritten = i2cWrite(REG_INT_EN, &value, sizeof(value));
    if (bytesWritten != sizeof(value)) {
        return -EIO;
    }

    // Issue a read to clear interrupt pin state
    int8_t bytesRead = i2cRead(REG_INT_EN, &value, sizeof(value));
    if (bytesRead != sizeof(value)) {
        return -EIO;
    }

    return 0;
}

int8_t moddoMOUSE::setDpiSettings(uint16_t x, uint16_t y)
{
    uint16_t dpi[2] = {x, y};

    if ((x < DPI_MIN) || (x > DPI_MAX)) {
        return -EINVAL;
    }
    if ((y < DPI_MIN) || (y > DPI_MAX)) {
        return -EINVAL;
    }

    int8_t bytesWritten = i2cWrite(REG_CPI_X, dpi, sizeof(dpi));
    if (bytesWritten != sizeof(dpi)) {
        return -EIO;
    }

    return 0;
}

int8_t moddoMOUSE::getDpiSettings(uint16_t *x, uint16_t *y)
{
    uint16_t dpi[2];

    int8_t bytesRead = i2cRead(REG_CPI_X, dpi, sizeof(dpi));
    if (bytesRead != sizeof(dpi)) {
        return -EIO;
    }

    if (x != NULL) {
        *x = dpi[0];
    }
    if (y != NULL) {
        *y = dpi[1];
    }

    return 0;
}

int8_t moddoMOUSE::setInvertSwapSettings(bool invert_x, bool invert_y, bool swap_xy)
{
    uint8_t value;

    int8_t bytesRead = i2cRead(REG_MOUSE_SETTINGS, &value, sizeof(value));
    if (bytesRead != sizeof(value)) {
        return -EIO;
    }

    if (invert_x) {
        value |= REG_MOUSE_SETTINGS_INVERT_X;
    } else {
        value &= ~REG_MOUSE_SETTINGS_INVERT_X;
    }
    if (invert_y) {
        value |= REG_MOUSE_SETTINGS_INVERT_Y;
    } else {
        value &= ~REG_MOUSE_SETTINGS_INVERT_Y;
    }
    if (swap_xy) {
        value |= REG_MOUSE_SETTINGS_SWAP_XY;
    } else {
        value &= ~REG_MOUSE_SETTINGS_SWAP_XY;
    }

    int8_t bytesWritten = i2cWrite(REG_MOUSE_SETTINGS, &value, sizeof(value));
    if (bytesWritten != sizeof(value)) {
        return -EIO;
    }

    return 0;
}

int8_t moddoMOUSE::getInvertSwapSettings(bool *invert_x, bool *invert_y, bool *swap_xy)
{
    uint8_t value;

    int8_t bytesRead = i2cRead(REG_MOUSE_SETTINGS, &value, sizeof(value));
    if (bytesRead != sizeof(value)) {
        return -EIO;
    }

    if (invert_x != NULL) {
        *invert_x = (value & REG_MOUSE_SETTINGS_INVERT_X);
    }
    if (invert_y != NULL) {
        *invert_y = (value & REG_MOUSE_SETTINGS_INVERT_Y);
    }
    if (swap_xy != NULL) {
        *swap_xy = (value & REG_MOUSE_SETTINGS_SWAP_XY);
    }

    return 0;
}

int8_t moddoMOUSE::setPollingRate(enum pollingRateOptions pollingRate)
{
    if (pollingRate > POLLING_RATE_MAX) {
        return -EINVAL;
    }

    uint8_t value;

    int8_t bytesRead = i2cRead(REG_MOUSE_SETTINGS, &value, sizeof(value));
    if (bytesRead != sizeof(value)) {
        return -EIO;
    }

    value &= ~REG_MOUSE_SETTINGS_POLLING_MASK;
    value |= (uint8_t)pollingRate;

    int8_t bytesWritten = i2cWrite(REG_MOUSE_SETTINGS, &value, sizeof(value));
    if (bytesWritten != sizeof(value)) {
        return -EIO;
    }

    return 0;
}

int8_t moddoMOUSE::getPollingRate(enum pollingRateOptions *pollingRate)
{
    if (pollingRate == NULL) {
        return -EINVAL;
    }

    uint8_t value;

    int8_t bytesRead = i2cRead(REG_MOUSE_SETTINGS, &value, sizeof(value));
    if (bytesRead != sizeof(value)) {
        return -EIO;
    }

    if (value > POLLING_RATE_MAX) {
        // Just cap at max value
        value = POLLING_RATE_MAX;
    }
    *pollingRate = (enum pollingRateOptions)(value & REG_MOUSE_SETTINGS_POLLING_MASK);

    return 0;
}

int8_t moddoMOUSE::setLiftDistance(enum liftDistanceOptions liftDistance)
{
    if (liftDistance > LIFT_DISTANCE_MAX) {
        return -EINVAL;
    }

    uint8_t value;
    switch(liftDistance) {
    case LIFT_DISTANCE_1MM:
        value = 1;
        break;
    case LIFT_DISTANCE_2MM:
        value = 2;
        break;
    default:
        return -EINVAL;
    }

    int8_t bytesWritten = i2cWrite(REG_LIFT_DISTANCE, &value, sizeof(value));
    if (bytesWritten != sizeof(value)) {
        return -EIO;
    }

    return 0;
}

int8_t moddoMOUSE::getLiftDistance(enum liftDistanceOptions *liftDistance)
{
    if (liftDistance == NULL) {
        return -EINVAL;
    }

    uint8_t value;

    int8_t bytesRead = i2cRead(REG_LIFT_DISTANCE, &value, sizeof(value));
    if (bytesRead != sizeof(value)) {
        return -EIO;
    }

    switch(value) {
    case 1:
        *liftDistance = LIFT_DISTANCE_1MM;
        break;
    case 2:
        *liftDistance = LIFT_DISTANCE_2MM;
        break;
    default:
        // cap at max
        *liftDistance = LIFT_DISTANCE_MAX;
        break;
    }

    return 0;
}

int8_t moddoMOUSE::setAngleTune(int8_t angle)
{
    if ((angle < ANGLE_TUNE_MIN) || (angle > ANGLE_TUNE_MAX)) {
        return -EINVAL;
    }

    int8_t bytesWritten = i2cWrite(REG_ANGLE_TUNE, &angle, sizeof(angle));
    if (bytesWritten != sizeof(angle)) {
        return -EIO;
    }

    return 0;
}

int8_t moddoMOUSE::getAngleTune(int8_t *angle)
{
    if (angle == NULL) {
        return -EINVAL;
    }

    uint8_t value;

    int8_t bytesRead = i2cRead(REG_ANGLE_TUNE, &value, sizeof(value));
    if (bytesRead != sizeof(value)) {
        return -EIO;
    }

    *angle = (int8_t)value;

    return 0;
}


int8_t moddoMOUSE::setVerticalWheel(int8_t delta)
{
    int8_t bytesWritten = i2cWrite(REG_V_WHEEL, &delta, sizeof(delta));
    if (bytesWritten != sizeof(delta)) {
        return -EIO;
    }

    return 0;
}

int8_t moddoMOUSE::getVerticalWheel(int8_t *delta)
{
    if (delta == NULL) {
        return -EINVAL;
    }

    uint8_t value;

    int8_t bytesRead = i2cRead(REG_V_WHEEL, &value, sizeof(value));
    if (bytesRead != sizeof(value)) {
        return -EIO;
    }

    *delta = (int8_t)value;

    return 0;
}

int8_t moddoMOUSE::setHorizontalWheel(int8_t delta)
{
    int8_t bytesWritten = i2cWrite(REG_H_WHEEL, &delta, sizeof(delta));
    if (bytesWritten != sizeof(delta)) {
        return -EIO;
    }

    return 0;
}

int8_t moddoMOUSE::getHorizontalWheel(int8_t *delta)
{
    if (delta == NULL) {
        return -EINVAL;
    }

    uint8_t value;

    int8_t bytesRead = i2cRead(REG_H_WHEEL, &value, sizeof(value));
    if (bytesRead != sizeof(value)) {
        return -EIO;
    }

    *delta = (int8_t)value;

    return 0;
}

// Initiate communication with the moddoMOUSE. Will return -EIO if communication failed.
int8_t moddoMOUSE::begin(uint8_t address, TwoWire& wirePort)
{
    i2cAddress = address;
    i2cPort = &wirePort;

    i2cPort->begin();

    // Read moddoMOUSE product ID
    uint8_t prodId;
    int8_t ret = readProductID(&prodId);
    if (ret != 0) {
        return -EIO;
    }

    if (prodId != MODDOMOUSE_PRODUCT_ID) {
        // Unexpected product ID
        return -EIO;
    }

    // Reset any previously set interrupts
    ret = disableAllInterrupts();
    if (ret != 0) {
        return -EIO;
    }

    return 0;
}
