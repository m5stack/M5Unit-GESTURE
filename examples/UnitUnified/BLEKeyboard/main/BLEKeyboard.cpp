/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example of sending identified gestures as keyboard commands via BLE

  The BLE HID keyboard is implemented directly on NimBLE-Arduino (NimBLEHIDDevice).

  Required:
  - https://github.com/h2zero/NimBLE-Arduino
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedGESTURE.h>
#include <wiring/m5_unit_unified_wiring.hpp>  // wiring::addI2C / failStop

#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>

using gesture_t = m5::unit::paj7620u2::Gesture;

namespace {
auto& lcd = M5.Display;

m5::unit::UnitUnified Units;
m5::unit::UnitGesture unit;

constexpr char DEVICE_NAME[]  = "PagerKB";
constexpr char MANUFACTURER[] = "M5UU";

// Keyboard usages (USB HID Usage Tables, Keyboard/Keypad page 0x07)
constexpr uint8_t HID_USAGE_DOWN_ARROW{0x51};
constexpr uint8_t HID_USAGE_UP_ARROW{0x52};

constexpr uint8_t REPORT_ID_KEYBOARD{1};
// Keyboard report descriptor, written from the boot keyboard descriptor of the USB HID 1.11 specification
// (Appendix B.1) with a report ID added. Not const because NimBLEHIDDevice::setReportMap() takes uint8_t*
uint8_t report_map[] = {
    0x05, 0x01,                // Usage Page (Generic Desktop)
    0x09, 0x06,                // Usage (Keyboard)
    0xA1, 0x01,                // Collection (Application)
    0x85, REPORT_ID_KEYBOARD,  //   Report ID
    0x05, 0x07,                //   Usage Page (Keyboard/Keypad)
    0x19, 0xE0,                //   Usage Minimum (Left Control)
    0x29, 0xE7,                //   Usage Maximum (Right GUI)
    0x15, 0x00,                //   Logical Minimum (0)
    0x25, 0x01,                //   Logical Maximum (1)
    0x75, 0x01,                //   Report Size (1)
    0x95, 0x08,                //   Report Count (8)
    0x81, 0x02,                //   Input (Data, Variable, Absolute): modifier keys
    0x95, 0x01,                //   Report Count (1)
    0x75, 0x08,                //   Report Size (8)
    0x81, 0x01,                //   Input (Constant): reserved byte
    0x95, 0x05,                //   Report Count (5)
    0x75, 0x01,                //   Report Size (1)
    0x05, 0x08,                //   Usage Page (LEDs)
    0x19, 0x01,                //   Usage Minimum (Num Lock)
    0x29, 0x05,                //   Usage Maximum (Kana)
    0x91, 0x02,                //   Output (Data, Variable, Absolute): LEDs
    0x95, 0x01,                //   Report Count (1)
    0x75, 0x03,                //   Report Size (3)
    0x91, 0x01,                //   Output (Constant): LED padding
    0x95, 0x06,                //   Report Count (6)
    0x75, 0x08,                //   Report Size (8)
    0x15, 0x00,                //   Logical Minimum (0)
    0x25, 0x65,                //   Logical Maximum (101)
    0x05, 0x07,                //   Usage Page (Keyboard/Keypad)
    0x19, 0x00,                //   Usage Minimum (0)
    0x29, 0x65,                //   Usage Maximum (101)
    0x81, 0x00,                //   Input (Data, Array): key codes
    0xC0,                      // End Collection
};

NimBLEHIDDevice* hid{};
NimBLECharacteristic* input_report{};

void ble_keyboard_begin()
{
    NimBLEDevice::init(DEVICE_NAME);
    // HID hosts require an encrypted, bonded link. "Just Works" pairing: bonding without a PIN
    NimBLEDevice::setSecurityAuth(true, false, false);

    auto server = NimBLEDevice::createServer();
    server->advertiseOnDisconnect(true);

    hid = new NimBLEHIDDevice(server);
    hid->setManufacturer(MANUFACTURER);
    hid->setPnp(0x02, 0x0000, 0x0000, 0x0100);  // USB vendor ID source, placeholder vendor / product IDs
    hid->setHidInfo(0x00, 0x02);                // No country code, normally connectable
    hid->setReportMap(report_map, sizeof(report_map));
    input_report = hid->getInputReport(REPORT_ID_KEYBOARD);
    hid->getOutputReport(REPORT_ID_KEYBOARD);  // LED state written by the host (not used)
    hid->setBatteryLevel(100);
    server->start();

    auto adv = NimBLEDevice::getAdvertising();
    adv->setAppearance(HID_KEYBOARD);
    adv->addServiceUUID(hid->getHidService()->getUUID());
    adv->setName(DEVICE_NAME);
    adv->start();
}

bool ble_keyboard_connected()
{
    return NimBLEDevice::getServer()->getConnectedCount() > 0;
}

// Press and release a key
void ble_keyboard_tap(const uint8_t usage)
{
    // Modifiers, reserved, then up to 6 key codes
    uint8_t report[8]{0, 0, usage};
    input_report->setValue(report, sizeof(report));
    input_report->notify();
    m5::utility::delay(10);

    uint8_t release[8]{};
    input_report->setValue(release, sizeof(release));
    input_report->notify();
}

bool inactive{};                                            // In the continuous input prevention period
m5::utility::elapsed_time_t inactive_at{};                  // When the last key was sent
constexpr m5::utility::elapsed_time_t INACTIVE_TIME{1500};  // Period of inactivity (ms)

constexpr const char* gstr[] = {
    "None", "Up",       "Down",      "Left",          "Right",   "Forward", "Backward", "Clockwise", "CounterClockwise",
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
    0,                     // None
    0,                     // Up
    0,                     // Down
    0,                     // Left
    0,                     // Right
    0,                     // Forward
    0,                     // Backward
    HID_USAGE_DOWN_ARROW,  // Clockwise
    HID_USAGE_UP_ARROW,    // CounterClockwise
    0,                     // Wave
    0,                     // Approach
    0,                     // HasObject
    0,                     // WakeupTrigger
    0,                     // Confirm
    0,                     // Abort
    0,                     // Reserve
    0,                     // NoObject
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
    M5.setTouchButtonHeightByRatio(100);

    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

    // Board-aware connection: NessoN1 -> SoftwareI2C on GROVE, NanoC6/NanoH2 -> M5.Ex_I2C, others -> Wire
    if (!m5::unit::wiring::addI2C(Units, unit) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        m5::unit::wiring::failStop();
    }

    M5_LOGI("M5UnitUnified initialized");
    M5_LOGI("%s", Units.debugInfo().c_str());

    lcd.fillScreen(TFT_DARKGRAY);
    ble_keyboard_begin();
}

void loop()
{
    static bool connected{};

    M5.update();
    Units.update();
    if (connected != ble_keyboard_connected()) {
        connected = ble_keyboard_connected();
        M5.Log.printf("Change BLE connection:%u\n", connected);
        lcd.fillScreen(connected ? TFT_DARKGREEN : TFT_DARKGRAY);
    }
    if (connected) {
        if (inactive) {
            if (!m5::utility::hasElapsed(inactive_at, INACTIVE_TIME)) {
                return;
            }
            inactive = false;
            lcd.fillScreen(TFT_DARKGREEN);
        }

        if (unit.updated()) {
            auto key = gesture_to_key(unit.gesture());
            if (key) {
                M5.Log.printf("Send [0X%X] Gesture:%s\n", key, gesture_to_string(unit.gesture()));
                ble_keyboard_tap(key);
                // Continuous input prevention period
                inactive    = true;
                inactive_at = m5::utility::millis();
                lcd.fillScreen(TFT_ORANGE);
            }
        }
    } else {
        m5::utility::delay(1000);
    }
}
