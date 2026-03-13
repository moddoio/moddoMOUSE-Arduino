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

#ifndef MODDOMOUSE_REGS_H
#define MODDOMOUSE_REGS_H

enum register_index {
    // Unique device identifier
    REG_DEVICE_ID,
    REG_DEVICE_ID_L,

    // Product ID (0x01 for moddoMOUSE)
    REG_PROD_ID,

    // Not currently used
    REG_STATUS,

    // Battery and charger status, usually read all in one transfer
    // Can enable REG_REG_INT_EN_BATTERY_CHANGE to get an interrupt when any of these change
    REG_BATTERY_VOLTAGE,
    REG_BATTERY_VOLTAGE_L,
    REG_BATTERY_CAPACITY,
    REG_BATTERY_CHARGER_STATUS,
    // End of battery and charger status

    REG_MOUSE_SETTINGS,

    // Motion delta accumulated value. Will accumulate until read
    // Must read all 4 bytes in one transaction
    REG_MOUSE_X,
    REG_MOUSE_X_L,
    REG_MOUSE_Y,
    REG_MOUSE_Y_L,
    // End of motion registers

    // Angle Tune
    REG_ANGLE_TUNE,

    // Main buttons state
    REG_MAIN_BUTTONS,

    // Vertical wheel accumulated value. Will accumulate until read/write
    REG_V_WHEEL,
    // Horizontal wheel accumulated value. Will accumulate until read/write
    REG_H_WHEEL,

    // Interrupt enable bits
    // Must set the appropriate bits to receive an interrupt on the INT pin
    REG_INT_EN,

    // Setting: lift distance
    REG_LIFT_DISTANCE,

    // Setting: DPI/CPI (Counts Per Inch)
    REG_CPI_X,
    REG_CPI_X_L,
    REG_CPI_Y,
    REG_CPI_Y_L,

    // Extra buttons from the expansion boards. Write bits to send keyboard key presses to the PC
    REG_EXP_BOARD_BUTTONS,
    REG_EXP_BOARD_BUTTONS_L,

    NUM_REGISTERS,
};

#define MODDOMOUSE_PRODUCT_ID 0x01

#ifndef BIT
#define BIT(n)  (1 << (n))
#endif

// REG_MOUSE_SETTINGS bits
#define REG_MOUSE_SETTINGS_POLLING_MASK 0x3
// bit 2 reserved
#define REG_MOUSE_SETTINGS_INVERT_X BIT(3)
#define REG_MOUSE_SETTINGS_INVERT_Y BIT(4)
#define REG_MOUSE_SETTINGS_SWAP_XY  BIT(5)

// REG_MAIN_BUTTONS bits
#define REG_MAIN_BUTTONS_LEFT_CLICK    BIT(0)
#define REG_MAIN_BUTTONS_RIGHT_CLICK   BIT(1)
#define REG_MAIN_BUTTONS_MIDDLE_CLICK  BIT(2)
#define REG_MAIN_BUTTONS_BACK_CLICK    BIT(3)
#define REG_MAIN_BUTTONS_FORWARD_CLICK BIT(4)

// REG_INT_EN bits
#define REG_REG_INT_EN_WHEEL          BIT(0)
#define REG_REG_INT_EN_MOTION         BIT(1)
#define REG_REG_INT_EN_MAIN_BUTTONS   BIT(2)
#define REG_REG_INT_EN_FAULTS         BIT(3)
#define REG_REG_INT_EN_BATTERY_CHANGE BIT(4)

#endif
