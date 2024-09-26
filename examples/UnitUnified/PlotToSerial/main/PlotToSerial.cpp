/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitGESTURE
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedGESTURE.h>
#include <M5Utility.h>

namespace {
auto& lcd = M5.Display;

m5::unit::UnitUnified Units;
m5::unit::UnitGESTURE unit;

using gesture_t = m5::unit::paj7620u2::Gesture;

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

using namespace m5::unit::paj7620u2;
Mode& operator++(Mode& m)
{
    uint8_t v = m5::stl::to_underlying(m) + 1;
    if (v > m5::stl::to_underlying(Mode::Cursor)) {
        v = 0;
    }
    m = static_cast<Mode>(v);
    return m;
}
Mode detection{Mode::Gesture};

enum class Corner : uint8_t {
    None,
    LeftTop,
    RightTop,
    LeftBottom,
    RightBottom,
    Center,
};
constexpr const char* cstr[] = {
    "None", "LeftTop", "RightTop", "LeftBottom", "RightBottom", "Center",
};

Corner detectCorner()
{
    bool exists{};
    uint16_t x{}, y{};

    if (unit.existsObject(exists) && unit.readObjectCenter(x, y) && exists) {
        // Determined by upper 5 bits
        x >>= 8;
        y >>= 8;
        //        M5_LOGW("%d:(%u,%u)", exists, x, y);
        if (x >= 9 && y <= 5) {
            return Corner::LeftBottom;
        }
        if (x >= 9 && y >= 9) {
            return Corner::RightBottom;
        }
        if (x <= 5 && y <= 5) {
            return Corner::LeftTop;
        }
        if (x <= 5 && y >= 9) {
            return Corner::RightTop;
        }
        return Corner::Center;
    }
    return Corner::None;
}

}  // namespace

void setup()
{
    m5::utility::delay(2000);

    M5.begin();

    auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
    auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
    M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
    Wire.begin(pin_num_sda, pin_num_scl, 400 * 1000U);

    if (!Units.add(unit, Wire) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        lcd.clear(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }

    M5_LOGI("M5UnitUnified has been begun");
    M5_LOGI("%s", Units.debugInfo().c_str());

    lcd.clear(TFT_DARKGREEN);
}

void loop()
{
    M5.update();
    Units.update();
    switch (unit.mode()) {
        case m5::unit::paj7620u2::Mode::Gesture: {
            static uint8_t noobj{};

            if (unit.updated()) {
                uint8_t nomot{};
                uint16_t size{};
                uint16_t x{}, y{};
                unit.readNoMotionCount(nomot);
                unit.readObjectSize(size);
                unit.readObjectCenter(x, y);

                M5_LOGI("Gesture:%s noobject:%u nomotion:%u size:%u (%u,%u)", gesture_to_string(unit.gesture()), noobj,
                        nomot, size, x, y);
            }
            unit.readNoObjectCount(noobj);

            static Corner pc{};
            Corner c = detectCorner();
            if (c != pc) {
                M5_LOGI("Obj:%s", cstr[(uint8_t)c]);
                pc = c;
            }
        } break;
        case m5::unit::paj7620u2::Mode::Proximity: {
            if (unit.updated()) {
                M5_LOGI("%s brightness:%u approch:%u", gesture_to_string(unit.gesture()), unit.brightness(),
                        unit.approach());
            }
        } break;
        case m5::unit::paj7620u2::Mode::Cursor: {
            if (unit.updated()) {
                M5_LOGI("Cursor:%u,%u", unit.cursorX(), unit.cursorY());
            }
            delay(100);
        } break;
        default:
            break;
    }
    if (M5.BtnA.wasClicked()) {
        auto prev = detection;
        ++detection;
        if (unit.writeMode(detection)) {
            M5_LOGI(">>> writeNMode %x", detection);
            switch (unit.mode()) {
                case m5::unit::paj7620u2::Mode::Gesture:
                    unit.writeFrequency(Frequency::Gaming);
                    break;
                case m5::unit::paj7620u2::Mode::Proximity:
                    unit.writeApproachThreshold(20, 10);
                    break;
                case m5::unit::paj7620u2::Mode::Cursor:
                    unit.writeFrequency(Frequency::Gaming);
                    break;
                default:
                    break;
            }
        } else {
            M5_LOGE("Failed to writeMode %x", detection);
            detection = prev;
        }
    }
}
