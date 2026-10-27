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
 * Serial moves to D5/D6, which this kit uses.
 *
 * Wiring (pinch pin: connected to)
 *   D0, D1   I2C to the mouse (SCL, SDA)
 *   D2       Scroll wheel click (closes to GND)
 *   D3       Scroll wheel encoder B
 *   D4       Interrupt from the mouse (BACK_INT). Not used here: it's free for your
 *            own sketches, like battery_monitoring and button_monitoring
 *   D5       Joystick press (closes to GND)
 *   D6       Scroll wheel encoder A
 *   A0, A1   Joystick Y, X. The joystick itself runs straight off VDD, so it's
 *            powered whenever the mouse powers the kit.
 *   A2-A4    Keypad rows 0-2, each a resistor ladder fed from VDD (see "Keypad
 *            ladders" below)
 *
 * Keypad keys and their button numbers in moddoHUB, keypad seen from above:
 *
 *            col 0  col 1  col 2  col 3
 *   row 0      0      1      2      3
 *   row 1      8      9     10     11
 *   row 2     16     17     18     19
 *
 *   Scroll wheel click: 7 (top right of the grid)
 *
 * moddoHUB shows the 32 expansion board buttons as rows of 8, so the keypad is the
 * top-left 4x3 block of the Back Board grid, laid out like the keys. The 9-key
 * keypad has no column 3, so it uses 0-2, 8-10 and 16-18 with no change here.
 *
 * With the mouse's default key map, row 0 types Q R S T, row 1 Y Z 1 2, row 2
 * 7 8 9 0, and the wheel click types X. To change one: open moddoHUB with the
 * mouse connected, click its button number in the Back Board grid, then press
 * the keyboard key it should send and confirm. To make it a controller button
 * instead, turn on Controller in that dialog and pick one. The joystick and
 * controller buttons work over USB and 2.4G, not Bluetooth.
 *
 * Any combination of keys can be held at once, all 12 included. Each row is one
 * analog pin, and every key in it pulls the pin down through its own resistor
 * (weighted 1:2:4:8), so each of the 16 combinations in a row reads as its own
 * voltage.
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


// Pin mappings. Wheel A and the mouse's interrupt are on pins with their own
// interrupt lines (EXTINT3 and EXTINT1 on the pinch), so they can wake it.
// Everything else is read on each tick.
// D5 is also the SAMD11's SWCLK: if it's low as the pinch comes out of reset, the
// chip waits for a debugger instead of starting, so the kit won't start if the
// joystick is held pressed while the mouse powers it up.
#define WHEEL_CLICK_PIN     2
#define WHEEL_B_PIN         3
#define INTERRUPT_PIN       4   // from the mouse, not used here
#define JOYSTICK_BUTTON_PIN 5
#define WHEEL_A_PIN         6
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
#define WHEEL_CLICK_BIT  7

// Keypad ladders. Each row:
//
//   VDD --22k--+------ row pin (A2, A3 or A4) --1nF-- GND
//              +--300k-- col 0 key -- GND
//              +--150k-- col 1 key -- GND
//              +--75k--- col 2 key -- GND
//              +--39k--- col 3 key -- GND
//
// With every key open no current flows, so the ladders stay powered all the time;
// a row draws at most ~70uA with all four keys held. Each key adds its own
// current, so every combination pulls the row pin to its own share of VDD, from
// 100% with no key down to 49% with all four. The closest two are only 1.76% of
// VDD apart: 1% resistors move a level by up to 0.5%, which still fits, and 0.1%
// ones leave more room. Each reading is matched to
// the nearest expected level, worked out from these values, so change them to
// match your board.
#define LADDER_PULLUP_OHMS 22000
const uint32_t LADDER_KEY_OHMS[KEYPAD_COLUMNS] = {300000, 150000, 75000, 39000};
#define LADDER_COMBINATIONS (1 << KEYPAD_COLUMNS)
// Each row is read this many times and summed, to average out ADC noise
#define LADDER_READS 4

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
#define JOYSTICK_CENTER_TOLERANCE 800
// The stick doesn't quite reach the ends of the ADC range, so reach full
// deflection this many counts before them
#define JOYSTICK_END_MARGIN 128
// Magnetic (Hall or TMR) sticks need a few ms after power-up before their output
// is right, so wait this long before measuring the center
#define JOYSTICK_STARTUP_MS 10

#define ADC_MAX 4095            // readings are 12 bits (set in setupFastAnalogRead())
#define TICK_HZ 128             // 128Hz is the fastest wakeup frequency supported by the RTC
#define DEBOUNCE_SAMPLES 2      // a key must read the same on 2 ticks in a row (8-16ms)
#define REFRESH_TICKS TICK_HZ   // resend everything every second, in case the mouse restarted

moddoMOUSE mouse;

// Expected sum of LADDER_READS readings for each combination of keys held in a row
uint16_t ladderLevels[LADDER_COMBINATIONS];
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

Debouncer rowDebounce[KEYPAD_ROWS] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
Debouncer joystickButtonDebounce = {0, 0, 0};
Debouncer wheelClickDebounce = {0, 0, 0};

uint8_t wheelA;
volatile int8_t wheelDetents = 0;
volatile bool tickPending = true;

#if USE_LOW_POWER
void onTick()
{
    tickPending = true;
}
#endif

// Called on every edge of wheel pin A, and on each loop in case an edge was missed.
// The wheel's encoder (Alps EC10E) has 12 pulses and 24 detents per turn, so A
// changes exactly once between two detents, while B holds still. If A and B differ
// after the change the wheel turned one way, and if they match, the other way.
// Contact bounce on A counts back and forth, so it cancels out.
void onWheelEdge()
{
    uint8_t a = digitalRead(WHEEL_A_PIN);
    if (a == wheelA) {
        return;
    }
    wheelA = a;

    if (a != digitalRead(WHEEL_B_PIN)) {
        wheelDetents++;
    } else {
        wheelDetents--;
    }
}

// analogRead() defaults to a 94kHz ADC clock and the longest sample time, about
// 0.4ms per read. The 17 reads per tick would keep the pinch awake most of the
// time, so run the ADC at 1.5MHz. Its sample time is plenty for the joystick and
// the ladders' 1nF capacitors. Each read then takes well under 20us.
void setupFastAnalogRead()
{
    analogReadResolution(12);
#if defined(ARDUINO_MODDO_PINCH)
    analogRead(JOYSTICK_X_PIN); // the core sets up the ADC on the first read
    ADC->CTRLB.bit.PRESCALER = ADC_CTRLB_PRESCALER_DIV32_Val;
    while (ADC->STATUS.bit.SYNCBUSY);
    ADC->SAMPCTRL.reg = ADC_SAMPCTRL_SAMPLEN(15);
    while (ADC->STATUS.bit.SYNCBUSY);
#endif
}

// Expected level for each combination: the pull-up's conductance against the sum
// of the held keys'. Conductances are in nanosiemens so the integer math is exact
// enough and stays within 32 bits.
void setupLadderLevels()
{
    const uint32_t pullup = 1000000000UL / LADDER_PULLUP_OHMS;
    for (uint8_t keys = 0; keys < LADDER_COMBINATIONS; keys++) {
        uint32_t held = 0;
        for (uint8_t col = 0; col < KEYPAD_COLUMNS; col++) {
            if (keys & (1 << col)) {
                held += 1000000000UL / LADDER_KEY_OHMS[col];
            }
        }
        ladderLevels[keys] = (uint32_t)ADC_MAX * LADDER_READS * pullup / (pullup + held);
    }
}

// Returns the keys held in this row, one bit per column
int8_t readKeypadRow(uint8_t pin)
{
    // The first conversion after switching channels can still carry some of the
    // previous one, so throw it away
    analogRead(pin);

    uint16_t reading = 0;
    for (uint8_t i = 0; i < LADDER_READS; i++) {
        reading += analogRead(pin);
    }

    uint8_t keys = 0;
    uint16_t closest = UINT16_MAX;
    for (uint8_t combination = 0; combination < LADDER_COMBINATIONS; combination++) {
        uint16_t distance = abs((int32_t)reading - ladderLevels[combination]);
        if (distance < closest) {
            closest = distance;
            keys = combination;
        }
    }
    return keys;
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

    delay(JOYSTICK_STARTUP_MS);
    for (int i = 0; i < 8; i++) {
        x += analogRead(JOYSTICK_X_PIN);
        y += analogRead(JOYSTICK_Y_PIN);
    }
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

    pinMode(JOYSTICK_BUTTON_PIN, INPUT_PULLUP);
    pinMode(WHEEL_CLICK_PIN, INPUT_PULLUP);
    pinMode(WHEEL_A_PIN, WHEEL_PIN_MODE);
    pinMode(WHEEL_B_PIN, WHEEL_PIN_MODE);

    setupFastAnalogRead();
    setupLadderLevels();
    calibrateJoystick();

    // The wheel can turn faster than the ticks, so count it on every edge of A
    wheelA = digitalRead(WHEEL_A_PIN);
#if USE_LOW_POWER
    LowPower.attachInterruptWakeup(WHEEL_A_PIN, onWheelEdge, CHANGE);
    // Requires v0.5.0 of moddo SAMD boards package
    LowPower.attachInterruptWakeup(RTC_PERIODIC_WAKEUP, onTick, PinchRTC::PER_128HZ);
#else
    attachInterrupt(digitalPinToInterrupt(WHEEL_A_PIN), onWheelEdge, CHANGE);
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
    onWheelEdge(); // catch an edge of A that the interrupt missed
    // Only send the wheel while it rests in a detent (A and B match). Bounce, or a
    // nudge that springs back, counts back and forth between detents and has
    // cancelled out by then.
    int8_t detents = 0;
    if (wheelA == digitalRead(WHEEL_B_PIN)) {
        detents = wheelDetents;
        wheelDetents = 0;
    }
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
        // The analog reads can be noisy, so smooth them a little (about 30ms)
        smoothX4 += (int32_t)analogRead(JOYSTICK_X_PIN) - smoothX4 / 4;
        smoothY4 += (int32_t)analogRead(JOYSTICK_Y_PIN) - smoothY4 / 4;

        uint32_t buttonBits = 0;
        for (uint8_t row = 0; row < KEYPAD_ROWS; row++) {
            uint8_t keys = debounce(rowDebounce[row], readKeypadRow(KEYPAD_ROW_PINS[row]));
            for (uint8_t col = 0; col < KEYPAD_COLUMNS; col++) {
                if (keys & (1 << col)) {
                    buttonBits |= 1UL << KEYPAD_BUTTON_BIT(row, col);
                }
            }
        }
        if (debounce(wheelClickDebounce, !digitalRead(WHEEL_CLICK_PIN))) {
            buttonBits |= 1UL << WHEEL_CLICK_BIT;
        }

        uint16_t gamepadBits = 0;
        if (debounce(joystickButtonDebounce, !digitalRead(JOYSTICK_BUTTON_PIN))) {
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
    if (!tickPending) {
        // Sleep until the next RTC tick or wheel movement. Wheel steps held back
        // between detents go out on a later tick, once the wheel settles.
        mouse.suspend();
        LowPower.sleep();
        mouse.resume();
    }
#else
    delay(1000 / TICK_HZ);
    tickPending = true;
#endif
}
