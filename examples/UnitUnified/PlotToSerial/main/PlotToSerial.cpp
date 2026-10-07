/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitGESTURE

  Serial output is always produced. When the board has a screen, the latest gesture is drawn large at the top
  and the previous ones are listed below it. BtnA cycles the view (Gesture / Proximity / Cursor / Corner); in Proximity
  the brightness and the approach state are drawn, in Cursor the object position is drawn as a dot, and in Corner
  the area where the object is (corners or center, read with existsObject / readObjectCenter in Gesture mode) is drawn.
*/
#include <algorithm>
#include <cstdlib>
#include <cstdio>

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

// What this example shows. Corner is not a sensor mode: it reads the object center in Gesture mode
enum class View : uint8_t { Gesture, Proximity, Cursor, Corner };
View& operator++(View& v)
{
    uint8_t n = m5::stl::to_underlying(v) + 1;
    if (n > m5::stl::to_underlying(View::Corner)) {
        n = 0;
    }
    v = static_cast<View>(n);
    return v;
}
View view{View::Gesture};

constexpr const char* vstr[] = {"Gesture", "Proximity", "Cursor", "Corner"};
const char* view_to_string(const View v)
{
    const auto idx = m5::stl::to_underlying(v);
    return idx < m5::stl::size(vstr) ? vstr[idx] : "ERR";
}

// Sensor mode used by each view
Mode view_to_mode(const View v)
{
    switch (v) {
        case View::Proximity:
            return Mode::Proximity;
        case View::Cursor:
            return Mode::Cursor;
        default:
            return Mode::Gesture;
    }
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

// Draw a text line of the history size below the mode name
void draw_value_line(const uint32_t row, const char* text, const uint16_t color)
{
    const int32_t y = history_top + row * history_line_height;
    lcd.fillRect(0, y, lcd.width(), history_line_height, BG_COLOR);
    lcd.setTextSize(history_text_size);
    lcd.setTextColor(color);
    lcd.drawString(text, lcd.width() / 2, y);
}

void draw_proximity(const uint8_t brightness, const bool approach)
{
    char buf[24]{};
    lcd.startWrite();
    lcd.setTextDatum(top_center);
    snprintf(buf, sizeof(buf), "Bright:%3u", brightness);
    draw_value_line(0, buf, TFT_WHITE);

    // Brightness bar (0 - 255)
    const int32_t bar_x = lcd.width() / 8;
    const int32_t bar_w = lcd.width() - bar_x * 2;
    const int32_t bar_y = history_top + history_line_height;
    const int32_t bar_h = history_line_height - 2;
    const int32_t fill  = bar_w * brightness / 255;
    lcd.fillRect(bar_x, bar_y, fill, bar_h, TFT_ORANGE);
    lcd.fillRect(bar_x + fill, bar_y, bar_w - fill, bar_h, BG_COLOR);
    lcd.drawRect(bar_x, bar_y, bar_w, bar_h, TFT_WHITE);

    draw_value_line(2, approach ? "Approach" : "-", approach ? TFT_RED : TFT_LIGHTGRAY);
    lcd.endWrite();
}

//! Cursor field: the area below the coordinate line where the object position is drawn as a dot
int32_t field_x{}, field_y{}, field_w{}, field_h{}, dot_r{};
int32_t dot_x{-1}, dot_y{-1};
//! The cursor is the object center on the 30x30 sensor array in 1/128 pixel units (R_PositionResolution = 7),
//! so it ranges from 0 to 29 * 128 = 3712
constexpr uint16_t CURSOR_MAX{29 * 128};

void layout_cursor_field()
{
    dot_r = std::max<int32_t>(2, history_text_size * 2);
    // Square field, kept inside the screen and the bottom margin (round screen)
    const int32_t top    = history_top + history_line_height;
    const int32_t bottom = lcd.height() - current_top;
    const int32_t size   = std::max<int32_t>(0, std::min<int32_t>(lcd.width() - current_top * 2, bottom - top));
    field_w              = size;
    field_h              = size;
    field_x              = (lcd.width() - size) / 2;
    field_y              = top;
}

void draw_cursor(const uint16_t cx, const uint16_t cy)
{
    char buf[24]{};
    lcd.startWrite();
    lcd.setTextDatum(top_center);
    snprintf(buf, sizeof(buf), "X:%4u Y:%4u", cx, cy);
    draw_value_line(0, buf, TFT_WHITE);

    if (field_w > dot_r * 2) {
        if (dot_x >= 0) {
            lcd.fillCircle(dot_x, dot_y, dot_r, BG_COLOR);
        }
        const int32_t range = field_w - dot_r * 2 - 2;
        // Seen from the front of the unit, X runs from right to left (same as the object center in Gesture mode)
        dot_x = field_x + dot_r + 1 + range * (CURSOR_MAX - std::min(cx, CURSOR_MAX)) / CURSOR_MAX;
        dot_y = field_y + dot_r + 1 + range * std::min(cy, CURSOR_MAX) / CURSOR_MAX;
        lcd.drawRect(field_x, field_y, field_w, field_h, TFT_DARKGREY);
        lcd.fillCircle(dot_x, dot_y, dot_r, TFT_YELLOW);
    }
    lcd.endWrite();
}

enum class Corner : uint8_t {
    None,
    LeftTop,
    RightTop,
    LeftBottom,
    RightBottom,
    Center,
    TooClose,
};
constexpr const char* cstr[] = {
    "None", "LeftTop", "RightTop", "LeftBottom", "RightBottom", "Center", "TooClose",
};

// Classify the object center (readObjectCenter: 0 - 3712, 30x30 sensor array in 1/128 pixel units).
// Seen from the front of the unit in Gesture mode, X runs from right to left and Y from top to bottom.
constexpr uint16_t CENTER_MAX{29 * 128};
constexpr uint16_t CENTER_MID{CENTER_MAX / 2};
constexpr uint16_t CENTER_RANGE{600};  // Within +-600 of the middle on both axes is the center
constexpr uint16_t SIZE_FULL{900};     // The object covers the whole 30x30 array (too close)

Corner detectCorner(const bool exists, const uint16_t size, const uint16_t cx, const uint16_t cy)
{
    if (!exists) {
        return Corner::None;
    }
    if (size >= SIZE_FULL) {
        return Corner::TooClose;
    }
    const uint16_t h = CENTER_MAX - std::min(cx, CENTER_MAX);  // 0: left, CENTER_MAX: right
    const uint16_t v = std::min(cy, CENTER_MAX);               // 0: top, CENTER_MAX: bottom
    if (std::abs(static_cast<int32_t>(h) - CENTER_MID) <= CENTER_RANGE &&
        std::abs(static_cast<int32_t>(v) - CENTER_MID) <= CENTER_RANGE) {
        return Corner::Center;
    }
    const bool left = h < CENTER_MID;
    const bool top  = v < CENTER_MID;
    return top ? (left ? Corner::LeftTop : Corner::RightTop) : (left ? Corner::LeftBottom : Corner::RightBottom);
}

//! Corner view state
constexpr uint32_t CORNER_INTERVAL_MS{50};
m5::utility::elapsed_time_t corner_at{};
Corner last_corner{Corner::None};
uint16_t last_cx{0xFFFF}, last_cy{0xFFFF};

// Corner field (shares the area of the cursor field): 2x2 quadrants and a center box
void draw_corner_field(const Corner c)
{
    if (field_w <= 0) {
        return;
    }
    const int32_t half = field_w / 2;
    const int32_t cw   = field_w / 3;
    const int32_t cx   = field_x + (field_w - cw) / 2;
    const int32_t cy   = field_y + (field_h - cw) / 2;
    lcd.fillRect(field_x, field_y, field_w, field_h, BG_COLOR);
    switch (c) {
        case Corner::LeftTop:
            lcd.fillRect(field_x, field_y, half, half, TFT_YELLOW);
            break;
        case Corner::RightTop:
            lcd.fillRect(field_x + half, field_y, field_w - half, half, TFT_YELLOW);
            break;
        case Corner::LeftBottom:
            lcd.fillRect(field_x, field_y + half, half, field_h - half, TFT_YELLOW);
            break;
        case Corner::RightBottom:
            lcd.fillRect(field_x + half, field_y + half, field_w - half, field_h - half, TFT_YELLOW);
            break;
        case Corner::Center:
            lcd.fillRect(cx, cy, cw, cw, TFT_YELLOW);
            break;
        case Corner::TooClose:
            lcd.fillRect(field_x, field_y, field_w, field_h, TFT_ORANGE);
            break;
        default:
            break;
    }
    lcd.drawRect(field_x, field_y, field_w, field_h, TFT_DARKGREY);
    lcd.drawFastHLine(field_x, field_y + half, field_w, TFT_DARKGREY);
    lcd.drawFastVLine(field_x + half, field_y, field_h, TFT_DARKGREY);
    lcd.drawRect(cx, cy, cw, cw, TFT_DARKGREY);
}

void draw_corner(const Corner c, const uint16_t cx, const uint16_t cy)
{
    char buf[24]{};
    lcd.startWrite();
    lcd.setTextDatum(top_center);
    if (c == Corner::None) {
        draw_value_line(0, "No object", TFT_LIGHTGRAY);
    } else {
        snprintf(buf, sizeof(buf), "X:%4u Y:%4u", cx, cy);
        draw_value_line(0, buf, TFT_WHITE);
    }
    draw_corner_field(c);
    // Name of the area in the middle of the field
    if (field_w > 0) {
        lcd.setTextDatum(middle_center);
        lcd.setTextSize(history_text_size);
        lcd.setTextColor(c == Corner::None ? TFT_LIGHTGRAY : TFT_WHITE);
        lcd.drawString(cstr[m5::stl::to_underlying(c)], field_x + field_w / 2, field_y + field_h / 2);
    }
    lcd.endWrite();
}

// Show the view name and start a fresh history
void draw_view(const View v)
{
    current_label = nullptr;
    history_count = 0;
    dot_x         = -1;
    lcd.startWrite();
    lcd.fillScreen(BG_COLOR);
    lcd.setTextDatum(top_center);
    draw_current(view_to_string(v), TFT_CYAN);
    if (v == View::Cursor && field_w > 0) {
        lcd.drawRect(field_x, field_y, field_w, field_h, TFT_DARKGREY);
    }
    if (v == View::Corner) {
        draw_corner_field(Corner::None);
    }
    lcd.endWrite();
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

    // A board without a screen reports a 1x1 dummy display
    has_lcd = (lcd.width() > 8 && lcd.height() > 8);
    if (has_lcd) {
        layout();
        layout_cursor_field();
        draw_view(view);
    }
}

void loop()
{
    M5.update();
    Units.update();

    switch (view) {
        case View::Gesture: {
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
        } break;
        case View::Proximity: {
            // Detect proximity
            if (unit.updated()) {
                M5.Log.printf("%s brightness:%u approach:%u\n", gesture_to_string(unit.gesture()), unit.brightness(),
                              unit.approach());
                if (has_lcd) {
                    draw_proximity(unit.brightness(), unit.approach());
                }
            }
        } break;
        case View::Cursor: {
            // Detect cursor
            if (unit.updated()) {
                M5.Log.printf("Cursor:%u,%u\n", unit.cursorX(), unit.cursorY());
                if (has_lcd) {
                    draw_cursor(unit.cursorX(), unit.cursorY());
                }
            }
            m5::utility::delay(100);
        } break;
        case View::Corner: {
            // Detect the area where the object is (polled, as the object center is not part of the periodic data)
            if (m5::utility::hasElapsed(corner_at, CORNER_INTERVAL_MS)) {
                corner_at = m5::utility::millis();
                bool exists{};
                uint16_t cx{}, cy{};
                uint16_t size{};
                if (unit.existsObject(exists) &&
                    (!exists || (unit.readObjectCenter(cx, cy) && unit.readObjectSize(size)))) {
                    const Corner c = detectCorner(exists, size, cx, cy);
                    if (!exists) {
                        cx = cy = 0;
                    }
                    if (c != last_corner || cx != last_cx || cy != last_cy) {
                        last_corner = c;
                        last_cx     = cx;
                        last_cy     = cy;
                        M5.Log.printf("Corner:%s center:%u,%u size:%u\n", cstr[m5::stl::to_underlying(c)], cx, cy,
                                      size);
                        if (has_lcd) {
                            draw_corner(c, cx, cy);
                        }
                    }
                }
            }
        } break;
        default:
            break;
    }

    if (M5.BtnA.wasClicked()) {
        const auto prev = view;
        ++view;
        if (unit.writeMode(view_to_mode(view))) {
            M5.Log.printf(">> view %s (mode %x)\n", view_to_string(view), m5::stl::to_underlying(unit.mode()));
            last_corner = Corner::None;
            last_cx = last_cy = 0xFFFF;
            if (has_lcd) {
                draw_view(view);
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
            M5_LOGE("Failed to writeMode %x", m5::stl::to_underlying(view_to_mode(view)));
            view = prev;
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
    constexpr uint32_t FEED_INTERVAL_MS{2000};
    constexpr TickType_t FEED_SLEEP_TICKS{pdMS_TO_TICKS(5)};
    static uint32_t s_last_feed_ms{};
    const uint32_t now_ms{static_cast<uint32_t>(esp_timer_get_time() / 1000)};
    if (now_ms - s_last_feed_ms >= FEED_INTERVAL_MS) {
        s_last_feed_ms = now_ms;
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
