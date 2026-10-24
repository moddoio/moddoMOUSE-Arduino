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

/*
 * Back expansion kit: joystick, 12-key keypad and horizontal scroll wheel
 *
 * This is the sketch the pre-assembled back expansion kit (a pinch on the back
 * expansion base board) ships with. It turns:
 *   - the joystick into the left stick of a game controller, and pressing it into
 *     the left stick button (L3)
 *   - each keypad key into an expansion board button, which you can bind to a
 *     keyboard key or a controller button in the moddoHUB web app
 *   - the scroll wheel into horizontal scrolling, and its click into one more
 *     bindable button
 *
 * Board: moddo pinch, with moddo SAMD Boards 0.5.0 or newer (low power mode uses
 * its RTC wakeups). Leave Tools > USB Config at the default: with USB disabled,
 * Serial moves to D5/D6, which this kit uses for the wheel.
 *
 * Wiring (pinch pin: connected to)
 *   D0, D1   I2C to the mouse (SCL, SDA)
 *   D2       Sensor power: joystick VDD and the top of the three keypad ladders
 *   D3       Joystick press (closes to GND)
 *   D4       Scroll wheel encoder A
 *   D5       Scroll wheel click (closes to GND)
 *   D6       Scroll wheel encoder B
 *   A0, A1   Joystick Y, X
 *   A2-A4    Keypad rows 0-2 (resistor ladders, see "Keypad ladders" below)
 *
 * Keypad keys and their button numbers in moddoHUB, keypad seen from above:
 *
 *            col 0  col 1  col 2  col 3
 *   row 0      0      1      2      3
 *   row 1      8      9     10     11
 *   row 2     16     17     18     19
 *
 *   Scroll wheel click: 24
 *
 * moddoHUB shows the 32 expansion board buttons as rows of 8, so the keypad is the
 * top-left 4x3 block of the Back Board grid, laid out like the keys. The 9-key
 * keypad has no column 3, so it uses 0-2, 8-10 and 16-18 with no change here.
 *
 * With the mouse's default key map, row 0 types Q R S T, row 1 Y Z 1 2, row 2
 * 7 8 9 0, and the wheel click types Space. To change one: open moddoHUB with the
 * mouse connected, click its button number in the Back Board grid, then press
 * the keyboard key it should send and confirm. To make it a controller button
 * instead, turn on Controller in that dialog and pick one. The joystick and
 * controller buttons work over USB and 2.4G, not Bluetooth.
 *
 * Holding two keys in the same row: each row is one analog pin, and the keys in
 * a row are chained so that the key nearest the pin (the lowest column) cuts off
 * the ones after it. So:
 *   - keys in different rows always work together (up to 3 at once, one per row)
 *   - within one row, only the lowest-column key held is seen. Hold column 2, then
 *     press column 0: column 2 reads as released until column 0 is let go. Bind
 *     keys you need to hold together, like movement keys, in different rows.
 *   - a row never reports a key that isn't pressed
 */

#include <moddoMOUSE.h>

// Change this to 0 if you don't want low power support (easier when debugging).
// If you don't use it the Arduino power consumption will be ~200x higher at idle,
// and will drain the mouse battery.
#define USE_LOW_POWER_IF_SUPPORTED 1

#if USE_LOW_POWER_IF_SUPPORTED && defined(ARDUINO_MODDO_PINCH)
    // Polling on RTC ticks needs PinchLowPower from moddo SAMD boards 0.5.0 or newer
    #include <PinchLowPower.h>
    #define USE_LOW_POWER 1
#else
    #define USE_LOW_POWER 0
#endif

// Don't output on serial port in low power mode
bool serialOutput = !USE_LOW_POWER;


// Pin mappings. The wheel pins need their own interrupt lines (EXTINT1 and EXTINT3
// on the pinch) so no wheel step is missed. Everything else is read on each tick.
// D5 is also the SAMD11's SWCLK: if it's low as the pinch comes out of reset (the
// wheel click held down at power-up), the chip waits for a debugger instead of
// starting. The sensor power and wheel pins can sit low at reset, so a button
// goes there.
#define SENSOR_POWER_PIN    2
#define JOYSTICK_BUTTON_PIN 3
#define WHEEL_A_PIN         4
#define WHEEL_CLICK_PIN     5
#define WHEEL_B_PIN         6
#define JOYSTICK_Y_PIN      A0
#define JOYSTICK_X_PIN      A1
const uint8_t KEYPAD_ROW_PINS[] = {A2, A3, A4};

// Bit 10 is Left joystick down in the gamepad button map
#define JOYSTICK_BUTTON_BIT 10

// Expansion board button bits. moddoHUB shows them as rows of 8.
#define KEYPAD_ROWS      3
#define KEYPAD_COLUMNS   4
#define HUB_GRID_COLUMNS 8
#define KEYPAD_BUTTON_BIT(row, col) ((row) * HUB_GRID_COLUMNS + (col))
#define WHEEL_CLICK_BIT  24

// Keypad ladders. Each row:
//
//   D2 --22k--+------ row pin (A2, A3 or A4)
//             +-- col 0 key -- GND
//            7.5k
//             +-- col 1 key -- GND
//            15k
//             +-- col 2 key -- GND
//            43k
//             +-- col 3 key -- GND
//
// A key pulls the row pin down to a fixed share of D2: 0%, 25%, 51% or 75% for
// columns 0-3, and 100% with no key down. The thresholds are halfway between
// those and are worked out from these values, so change them to match your board.
#define LADDER_PULLUP_OHMS 22000
// The resistor before each column's tap, starting from the row pin
const uint32_t LADDER_STEP_OHMS[KEYPAD_COLUMNS] = {0, 7500, 15000, 43000};

// Change to INPUT if the board has its own pull-up resistors on the wheel pins:
// the internal ones draw ~150uA while the wheel rests with both contacts closed.
#define WHEEL_PIN_MODE INPUT_PULLUP
// Change to -1 if the wheel scrolls the wrong way
#define WHEEL_DIRECTION 1

// Change to 1 if an axis moves the wrong way in games
#define JOYSTICK_INVERT_X 0
#define JOYSTICK_INVERT_Y 0
// The stick's center is measured at startup. If it's further than this from
// mid-scale (someone was holding the stick), mid-scale is used instead.
#define JOYSTICK_CENTER_TOLERANCE 200
// The pots don't quite reach the ends of the ADC range, so reach full deflection
// this many counts before them
#define JOYSTICK_END_MARGIN 32

#define ADC_MAX 1023            // analogRead() is 10 bits by default
#define TICK_HZ 128             // 128Hz is the fastest wakeup frequency supported by the RTC
#define SENSOR_SETTLE_US 20     // settling time after powering the joystick and ladders
#define DEBOUNCE_SAMPLES 2      // a key must read the same on 2 ticks in a row (8-16ms)
#define REFRESH_TICKS TICK_HZ   // resend everything every second, in case the mouse restarted

#define NO_KEY -1

moddoMOUSE mouse;

uint16_t ladderThresholds[KEYPAD_COLUMNS];
int32_t joystickCenterX = (ADC_MAX + 1) / 2;
int32_t joystickCenterY = (ADC_MAX + 1) / 2;
// Smoothed joystick readings, in quarter ADC counts so the averaging stays accurate
int32_t smoothX4;
int32_t smoothY4;

struct Debouncer {
    int8_t stable;
    int8_t candidate;
    uint8_t count;
};

Debouncer rowDebounce[KEYPAD_ROWS] = {{NO_KEY, NO_KEY, 0}, {NO_KEY, NO_KEY, 0}, {NO_KEY, NO_KEY, 0}};
Debouncer joystickButtonDebounce = {0, 0, 0};
Debouncer wheelClickDebounce = {0, 0, 0};

uint8_t wheelState;
volatile int8_t wheelDetents = 0;
volatile bool tickPending = true;

#if USE_LOW_POWER
void onTick()
{
    tickPending = true;
}
#endif

uint8_t readWheel()
{
    return (digitalRead(WHEEL_A_PIN) << 1) | digitalRead(WHEEL_B_PIN);
}

// Called on every edge of either wheel pin. The wheel's encoder (Alps EC10E) has
// 12 pulses and 24 detents per turn, so it rests in a detent whenever A and B
// match (both open or both closed), two quadrature steps apart.
void onWheelEdge()
{
    // Indexed by previous state * 4 + new state: +1 or -1 for a step, 0 for no
    // change or a jump over a state (bounce or a missed edge), which is safer to drop
    static const int8_t QUADRATURE_STEPS[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
    static int8_t steps = 0;

    uint8_t state = readWheel();
    steps += QUADRATURE_STEPS[(wheelState << 2) | state];
    wheelState = state;

    if (state == 0b00 || state == 0b11) {
        if (steps >= 2) {
            wheelDetents++;
        } else if (steps <= -2) {
            wheelDetents--;
        }
        // Anything else was bounce that came back, or a missed edge. Either way,
        // start the next detent from here.
        steps = 0;
    }
}

// analogRead() defaults to a 94kHz ADC clock and the longest sample time, about
// 0.4ms per read. Five reads per tick would keep the pinch awake a quarter of the
// time, so run the ADC at 1.5MHz with a sample time that still fully charges
// through the ladders. Each read then takes well under 20us.
void setupFastAnalogRead()
{
#if defined(ARDUINO_MODDO_PINCH)
    analogRead(JOYSTICK_X_PIN); // the core sets up the ADC on the first read
    ADC->CTRLB.bit.PRESCALER = ADC_CTRLB_PRESCALER_DIV32_Val;
    while (ADC->STATUS.bit.SYNCBUSY);
    ADC->SAMPCTRL.reg = ADC_SAMPCTRL_SAMPLEN(15);
    while (ADC->STATUS.bit.SYNCBUSY);
#endif
}

// Expected reading with the key in `col` down: the ladder below its tap against
// the pull-up. Column KEYPAD_COLUMNS means no key down.
uint16_t ladderLevel(int8_t col)
{
    if (col >= KEYPAD_COLUMNS) {
        return ADC_MAX;
    }
    uint32_t below = 0;
    for (int8_t i = 0; i <= col; i++) {
        below += LADDER_STEP_OHMS[i];
    }
    return ADC_MAX * below / (below + LADDER_PULLUP_OHMS);
}

void setupLadderThresholds()
{
    for (int8_t col = 0; col < KEYPAD_COLUMNS; col++) {
        ladderThresholds[col] = (ladderLevel(col) + ladderLevel(col + 1)) / 2;
    }
}

// Returns the column of the key held in this row, or NO_KEY
int8_t readKeypadRow(uint8_t pin)
{
    uint16_t reading = analogRead(pin);
    for (int8_t col = 0; col < KEYPAD_COLUMNS; col++) {
        if (reading < ladderThresholds[col]) {
            return col;
        }
    }
    return NO_KEY;
}

// Only accept a new reading once it has been the same for DEBOUNCE_SAMPLES ticks
int8_t debounce(Debouncer &d, int8_t reading)
{
    if (reading != d.candidate) {
        d.candidate = reading;
        d.count = 0;
    }
    if (d.count < DEBOUNCE_SAMPLES) {
        d.count++;
    }
    if (d.count >= DEBOUNCE_SAMPLES) {
        d.stable = reading;
    }
    return d.stable;
}

void calibrateJoystick()
{
    int32_t x = 0;
    int32_t y = 0;

    digitalWrite(SENSOR_POWER_PIN, 1);
    delayMicroseconds(SENSOR_SETTLE_US);
    for (int i = 0; i < 8; i++) {
        x += analogRead(JOYSTICK_X_PIN);
        y += analogRead(JOYSTICK_Y_PIN);
    }
    digitalWrite(SENSOR_POWER_PIN, 0);
    x /= 8;
    y /= 8;

    if (abs(x - joystickCenterX) <= JOYSTICK_CENTER_TOLERANCE) {
        joystickCenterX = x;
    }
    if (abs(y - joystickCenterY) <= JOYSTICK_CENTER_TOLERANCE) {
        joystickCenterY = y;
    }
    smoothX4 = joystickCenterX * 4;
    smoothY4 = joystickCenterY * 4;
}

// Scales each side of the center separately, so an off-center stick still
// reaches full deflection both ways
int8_t axisToGamepad(int32_t reading, int32_t center, bool invert)
{
    int32_t value;
    if (reading >= center) {
        value = map(reading, center, ADC_MAX - JOYSTICK_END_MARGIN, 0, JOYSTICK_MAX_VAL);
    } else {
        value = map(reading, JOYSTICK_END_MARGIN, center, JOYSTICK_MIN_VAL, 0);
    }
    value = constrain(value, JOYSTICK_MIN_VAL, JOYSTICK_MAX_VAL);
    return invert ? -value : value;
}

void setup()
{
    if (serialOutput) Serial.begin(9600);

    // Joystick and ladders are powered by a GPIO, so they only draw current while being read
    pinMode(SENSOR_POWER_PIN, OUTPUT);
    digitalWrite(SENSOR_POWER_PIN, 0);
    pinMode(JOYSTICK_BUTTON_PIN, INPUT_PULLUP);
    pinMode(WHEEL_CLICK_PIN, INPUT_PULLUP);
    pinMode(WHEEL_A_PIN, WHEEL_PIN_MODE);
    pinMode(WHEEL_B_PIN, WHEEL_PIN_MODE);

    setupFastAnalogRead();
    setupLadderThresholds();
    calibrateJoystick();

    // The wheel can turn faster than the ticks, so decode it on every edge
    wheelState = readWheel();
#if USE_LOW_POWER
    LowPower.attachInterruptWakeup(WHEEL_A_PIN, onWheelEdge, CHANGE);
    LowPower.attachInterruptWakeup(WHEEL_B_PIN, onWheelEdge, CHANGE);
    // Requires v0.5.0 of moddo SAMD boards package
    LowPower.attachInterruptWakeup(RTC_PERIODIC_WAKEUP, onTick, PinchRTC::PER_128HZ);
#else
    attachInterrupt(digitalPinToInterrupt(WHEEL_A_PIN), onWheelEdge, CHANGE);
    attachInterrupt(digitalPinToInterrupt(WHEEL_B_PIN), onWheelEdge, CHANGE);
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

void loop()
{
    static bool mouseConnected = false;
    static uint8_t ticksUntilRefresh = 0;
    static uint32_t lastButtonBits = 0;
    static uint16_t lastGamepadBits = 0;
    static int8_t lastX = 0;
    static int8_t lastY = 0;

    if (!mouseConnected) {
        mouseConnected = connect();
        if (!mouseConnected) {
            delay(100);
            return;
        }
        // Send everything on the next tick, but not wheel steps from while we were disconnected
        ticksUntilRefresh = 0;
        wheelDetents = 0;
    }

    noInterrupts(); // we need to do this operation atomically
    int8_t detents = wheelDetents;
    wheelDetents = 0;
    bool tick = tickPending;
    tickPending = false;
    interrupts();

    if (detents != 0) {
        if (serialOutput) {
            Serial.print("Changing H wheel by ");
            Serial.println(WHEEL_DIRECTION * detents);
        }

        if (mouse.setHorizontalWheel(WHEEL_DIRECTION * detents) < 0) {
            if (serialOutput) Serial.println("Couldn't set H wheel: error");
            mouseConnected = false;
            return;
        }
    }

    if (tick) {
        // Power the joystick and ladders only while reading them
        digitalWrite(SENSOR_POWER_PIN, 1);
        delayMicroseconds(SENSOR_SETTLE_US);

        // The analog reads can be noisy, so smooth them a little (about 30ms)
        smoothX4 += (int32_t)analogRead(JOYSTICK_X_PIN) - smoothX4 / 4;
        smoothY4 += (int32_t)analogRead(JOYSTICK_Y_PIN) - smoothY4 / 4;

        int8_t rowKeys[KEYPAD_ROWS];
        for (uint8_t row = 0; row < KEYPAD_ROWS; row++) {
            rowKeys[row] = readKeypadRow(KEYPAD_ROW_PINS[row]);
        }

        // Read while powered, in case the joystick board's ground is switched too
        bool joystickButton = !digitalRead(JOYSTICK_BUTTON_PIN);

        digitalWrite(SENSOR_POWER_PIN, 0);

        uint32_t buttonBits = 0;
        for (uint8_t row = 0; row < KEYPAD_ROWS; row++) {
            int8_t col = debounce(rowDebounce[row], rowKeys[row]);
            if (col != NO_KEY) {
                buttonBits |= 1UL << KEYPAD_BUTTON_BIT(row, col);
            }
        }
        if (debounce(wheelClickDebounce, !digitalRead(WHEEL_CLICK_PIN))) {
            buttonBits |= 1UL << WHEEL_CLICK_BIT;
        }

        uint16_t gamepadBits = 0;
        if (debounce(joystickButtonDebounce, joystickButton)) {
            gamepadBits |= 1 << JOYSTICK_BUTTON_BIT;
        }

        int8_t x = axisToGamepad(smoothX4 / 4, joystickCenterX, JOYSTICK_INVERT_X);
        int8_t y = axisToGamepad(smoothY4 / 4, joystickCenterY, JOYSTICK_INVERT_Y);

        // Only write what changed, plus everything once a second
        bool refresh = (ticksUntilRefresh == 0);
        ticksUntilRefresh = refresh ? REFRESH_TICKS : ticksUntilRefresh - 1;

        if (buttonBits != lastButtonBits || refresh) {
            if (mouse.setButtons(buttonBits) < 0) {
                if (serialOutput) Serial.println("Couldn't write buttons: error");
                mouseConnected = false;
                return;
            }
            if (serialOutput && buttonBits != lastButtonBits) {
                Serial.print("Wrote button register = ");
                Serial.println(buttonBits, HEX);
            }
            lastButtonBits = buttonBits;
        }

        if (gamepadBits != lastGamepadBits || refresh) {
            if (mouse.setGamepadButtons(gamepadBits) < 0) {
                if (serialOutput) Serial.println("Couldn't write gamepad buttons: error");
                mouseConnected = false;
                return;
            }
            lastGamepadBits = gamepadBits;
        }

        if (x != lastX || y != lastY || refresh) {
            if (mouse.setGamepadJoystick(0, x, y) < 0) {
                if (serialOutput) Serial.println("Couldn't write joystick: error");
                mouseConnected = false;
                return;
            }
            if (serialOutput && (x != lastX || y != lastY)) {
                Serial.print("Joystick x = ");
                Serial.print(x);
                Serial.print(", y = ");
                Serial.println(y);
            }
            lastX = x;
            lastY = y;
        }
    }

#if USE_LOW_POWER
    if (!tickPending && wheelDetents == 0) {
        // Sleep until the next RTC tick or wheel movement
        mouse.suspend();
        LowPower.sleep();
        mouse.resume();
    }
#else
    delay(1000 / TICK_HZ);
    tickPending = true;
#endif
}
