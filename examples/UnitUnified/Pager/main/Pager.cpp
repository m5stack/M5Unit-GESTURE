/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example of sending identified gestures as keyboard commands via BLE

  Required:
  - https://github.com/T-vK/ESP32-BLE-Keyboard
  - https://github.com/h2zero/NimBLE-Arduino
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedGESTURE.h>
#include <M5Utility.h>

#define USE_NIMBLE  // Define it if using NIMBLE
#include <BleKeyboard.h>

using gesture_t = m5::unit::paj7620u2::Gesture;

namespace {
auto& lcd = M5.Display;

m5::unit::UnitUnified Units;
m5::unit::UnitGESTURE unit;

BleKeyboard bleKeyboard{"PagerKB", "M5UU", 100};
unsigned long inactive_to{};
constexpr decltype(inactive_to) INACTIVE_TIME{1500};  // Period of inactivity (ms)

constexpr const char* gstr[] = {
    "None", "Left ",    "Right",     "Down",          "Up",      "Forward", "Backward", "Clockwise", "CounterClockwise",
    "Wave", "Approach", "HasObject", "WakeupTrigger", "Confirm", "Abort",   "Reserve",  "NoObject",
};
const char* gesture_to_string(const gesture_t g)
{
    auto gg = m5::stl::to_underlying(g);

    uint32_t idx = (gg == 0) ? 0 : __builtin_ctz(gg) + 1;
    return idx < m5::stl::size(gstr) ? gstr[idx] : "ERR";
}

// Gesture and keycode correspondence table
constexpr uint8_t key_table[] = {
    0,               // None
    0,               // Left
    0,               // Right
    0,               // Up
    0,               // Down
    0,               // Forward
    0,               // Backward
    KEY_DOWN_ARROW,  // Clockwise
    KEY_UP_ARROW,    // CounterClockwise
    0,               // Wave
    0,               // Approach
    0,               // HasObjectt
    0,               // WakeupTrigger
    0,               // Confirm
    0,               // Abort
    0,               // Reserve
    0,               // NoObject
};

uint8_t gesture_to_key(const gesture_t g)
{
    auto gg      = m5::stl::to_underlying(g);
    uint32_t idx = (gg == 0) ? 0 : __builtin_ctz(gg) + 1;
    return idx < m5::stl::size(key_table) ? key_table[idx] : 0;
}

}  // namespace

void setup()
{
    M5.begin();

    auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
    auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
    M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
    Wire.begin(pin_num_sda, pin_num_scl, 400 * 1000);

    if (!Units.add(unit, Wire) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        lcd.clear(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }

    M5_LOGI("M5UnitUnified has been begun");
    M5_LOGI("%s", Units.debugInfo().c_str());

    lcd.clear(TFT_DARKGRAY);
    bleKeyboard.begin();
}

void loop()
{
    static bool connected{};

    M5.update();
    Units.update();
    if (connected != bleKeyboard.isConnected()) {
        connected = bleKeyboard.isConnected();
        M5_LOGW("Change BLE connection:%u", connected);
        lcd.clear(connected ? TFT_DARKGREEN : TFT_DARKGRAY);
    }
    if (connected) {
        if (inactive_to) {
            if (m5::utility::millis() < inactive_to) {
                return;
            }
            inactive_to = 0;
            lcd.clear(TFT_DARKGREEN);
        }

        if (unit.updated()) {
            auto key = gesture_to_key(unit.gesture());
            if (key) {
                M5_LOGI("Send %x by %s", key, gesture_to_string(unit.gesture()));
                bleKeyboard.write(key);
                // Continuous input prevention period
                inactive_to = m5::utility::millis() + INACTIVE_TIME;
                lcd.clear(TFT_ORANGE);
            }
        }
    } else {
        m5::utility::delay(1000);
    }
}
