/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitGESTURE

  Serial output is always produced. When the board has a screen, the latest gesture is drawn large at the top
  and the previous ones are listed below it. BtnA cycles the mode (Gesture / Proximity / Cursor).
*/
#include <algorithm>

#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedGESTURE.h>
#include <wiring/m5_unit_unified_wiring.hpp>  // wiring::addI2C / failStop

namespace {
auto& lcd = M5.Display;

m5::unit::UnitUnified Units;
m5::unit::UnitGesture unit;

using gesture_t = m5::unit::paj7620u2::Gesture;

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

constexpr const char* mstr[] = {"Gesture", "Proximity", "Cursor"};
const char* mode_to_string(const Mode m)
{
    const auto idx = m5::stl::to_underlying(m);
    return idx < m5::stl::size(mstr) ? mstr[idx] : "ERR";
}

//! True when the board has a real screen (Atom / NanoC6 / NanoH2 / NessoN1 have none)
bool has_lcd{};

// RGB565 (uint16_t). M5GFX treats a uint32_t color as RGB888, so the TFT_* values must not be widened
constexpr uint16_t BG_COLOR{TFT_DARKGREEN};

//! Layout scaled by the screen size, so that the same code fits Stick (80 px high) up to Tab5 (720 px high)
int32_t history_text_size{}, current_text_size{};
int32_t current_top{}, current_height{};
int32_t history_top{}, history_line_height{};
constexpr uint32_t HISTORY_MAX{32};
uint32_t history_rows{}, history_count{};
const char* history[HISTORY_MAX]{};
const char* current_label{};

// Shorten the long names so that they fit on narrow screens
const char* gesture_label(const gesture_t g)
{
    switch (g) {
        case Gesture::Clockwise:
            return "CW";
        case Gesture::CounterClockwise:
            return "CCW";
        default:
            return gesture_to_string(g);
    }
}

void layout()
{
    const int32_t w = lcd.width();
    const int32_t h = lcd.height();
    // 6x8 font scaled by the screen size; the current gesture is twice the size of the history
    history_text_size = std::max<int32_t>(1, std::min(w / 160, h / 120));
    current_text_size = history_text_size * 2;
    // The top margin keeps the text inside a round screen (Dial)
    current_top         = h / 16;
    current_height      = 8 * current_text_size + 4 * history_text_size;
    history_top         = current_top + current_height;
    history_line_height = 8 * history_text_size + 2;
    history_rows =
        std::min<int32_t>(HISTORY_MAX, std::max<int32_t>(0, (h - current_top - history_top) / history_line_height));
}

void draw_current(const char* label, const uint16_t color)
{
    lcd.fillRect(0, current_top, lcd.width(), current_height, BG_COLOR);
    lcd.setTextSize(current_text_size);
    lcd.setTextColor(color);
    lcd.drawString(label, lcd.width() / 2, current_top);
}

void draw_history()
{
    lcd.fillRect(0, history_top, lcd.width(), history_rows * history_line_height, BG_COLOR);
    lcd.setTextSize(history_text_size);
    lcd.setTextColor(TFT_LIGHTGRAY);
    for (uint32_t i = 0; i < history_count; ++i) {
        lcd.drawString(history[i], lcd.width() / 2, history_top + i * history_line_height);
    }
}

void draw_gesture(const gesture_t g)
{
    // The previous gesture moves to the top of the history (newest first)
    if (current_label && history_rows) {
        history_count = std::min(history_count + 1, history_rows);
        for (uint32_t i = history_count - 1; i > 0; --i) {
            history[i] = history[i - 1];
        }
        history[0] = current_label;
    }
    current_label = gesture_label(g);

    lcd.startWrite();
    lcd.setTextDatum(top_center);
    draw_current(current_label, TFT_YELLOW);
    draw_history();
    lcd.endWrite();
}

// Show the mode name and start a fresh history
void draw_mode(const Mode m)
{
    current_label = nullptr;
    history_count = 0;
    lcd.startWrite();
    lcd.fillScreen(BG_COLOR);
    lcd.setTextDatum(top_center);
    draw_current(mode_to_string(m), TFT_CYAN);
    lcd.endWrite();
}

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

#if 0
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
#endif

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

    M5_LOGI("M5UnitUnified has been begun");
    M5_LOGI("%s", Units.debugInfo().c_str());

    // A board without a screen reports a 1x1 dummy display
    has_lcd = (lcd.width() > 8 && lcd.height() > 8);
    if (has_lcd) {
        layout();
        draw_mode(detection);
    }
}

void loop()
{
    M5.update();
    Units.update();

    switch (unit.mode()) {
        case m5::unit::paj7620u2::Mode::Gesture: {
            // Detect gesture
            static uint8_t noobj{};

            unit.readNoObjectCount(noobj);
            if (unit.updated()) {
                uint8_t nomot{};
                uint16_t size{};
                uint16_t x{}, y{};
                unit.readNoMotionCount(nomot);
                unit.readObjectSize(size);
                unit.readObjectCenter(x, y);
                const auto g = unit.gesture();
                if (g != Gesture::None) {
                    M5.Log.printf("Gesture:%s noobject:%u nomotion:%u size:%u (%u,%u)\n", gesture_to_string(g), noobj,
                                  nomot, size, x, y);
                    if (has_lcd) {
                        draw_gesture(g);
                    }
                }
            }
#if 0
            static Corner pc{};
            Corner c = detectCorner();
            if (c != pc) {
                M5.Log.printf("Obj:%s\n", cstr[(uint8_t)c]);
                pc = c;
            }
#endif
        } break;
        case m5::unit::paj7620u2::Mode::Proximity: {
            // Detect proximity
            if (unit.updated()) {
                M5.Log.printf("%s brightness:%u approach:%u\n", gesture_to_string(unit.gesture()), unit.brightness(),
                              unit.approach());
            }
        } break;
        case m5::unit::paj7620u2::Mode::Cursor: {
            // Detect cursor
            if (unit.updated()) {
                M5.Log.printf("Cursor:%u,%u\n", unit.cursorX(), unit.cursorY());
            }
            m5::utility::delay(100);
        } break;
        default:
            break;
    }

    if (M5.BtnA.wasClicked()) {
        auto prev = detection;
        ++detection;
        if (unit.writeMode(detection)) {
            M5.Log.printf(">> writeMode %x\n", detection);
            if (has_lcd) {
                draw_mode(detection);
            }
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

#if !defined(ARDUINO)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>

#if CONFIG_FREERTOS_UNICORE
// Single-core SoCs: run loop() back-to-back, but every ~2 s yield a 5 ms slice so the IDLE task
// runs and feeds the task watchdog (default 5 s).
static inline void feedIdleTaskPeriodically(void)
{
    constexpr uint32_t FEED_INTERVAL_MS   = 2000;
    constexpr TickType_t FEED_SLEEP_TICKS = pdMS_TO_TICKS(5);
    static uint32_t s_next_feed_ms        = 0;
    const uint32_t now_ms                 = static_cast<uint32_t>(esp_timer_get_time() / 1000);
    if (now_ms >= s_next_feed_ms) {
        s_next_feed_ms = now_ms + FEED_INTERVAL_MS;
        vTaskDelay(FEED_SLEEP_TICKS);
    }
}
#endif

extern "C" void app_main(void)
{
    setup();
    for (;;) {
#if CONFIG_FREERTOS_UNICORE
        feedIdleTaskPeriodically();
#endif
        loop();
    }
}
#endif
