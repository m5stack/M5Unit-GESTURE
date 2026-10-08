/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example of using UnitGesture (PAJ7620U2) via UnitPaHub

  Board ---> PaHub ---> ch:0 UnitGesture

  Detected gestures are printed to the serial output, and the latest one is also drawn when the board has a screen.
*/
#include <algorithm>

#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedGESTURE.h>
#include <M5UnitUnifiedHUB.h>                 // UnitPaHub
#include <wiring/m5_unit_unified_wiring.hpp>  // wiring::addI2C / failStop

namespace {
auto& lcd = M5.Display;

m5::unit::UnitUnified Units;
m5::unit::UnitPaHub hub;
m5::unit::UnitGesture unit;

constexpr uint8_t GESTURE_CHANNEL{0};  // PaHub channel the UnitGesture is connected to

using gesture_t = m5::unit::paj7620u2::Gesture;

constexpr const char* gstr[] = {
    "None", "Up",       "Down",      "Left",          "Right",   "Forward", "Backward", "Clockwise", "CounterClockwise",
    "Wave", "Approach", "HasObject", "WakeupTrigger", "Confirm", "Abort",   "Reserve",  "NoObject",
};
const char* gesture_to_string(const gesture_t g)
{
    const auto gg      = m5::stl::to_underlying(g);
    const uint32_t idx = (gg == 0) ? 0 : __builtin_ctz(gg) + 1;
    return idx < m5::stl::size(gstr) ? gstr[idx] : "ERR";
}

//! True when the board has a real screen (Atom / NanoC6 / NanoH2 / NessoN1 have none)
bool has_lcd{};

void draw_gesture(const char* label)
{
    lcd.startWrite();
    lcd.fillScreen(TFT_DARKGREEN);
    lcd.setTextDatum(middle_center);
    lcd.setTextColor(TFT_YELLOW);
    // 6x8 font scaled so that the longest name ("CounterClockwise", 16 chars) fits the width
    lcd.setTextSize(std::max<int32_t>(1, lcd.width() / (16 * 6)));
    lcd.drawString(label, lcd.width() / 2, lcd.height() / 2);
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

    if (!hub.add(unit, GESTURE_CHANNEL)) {  // PaHub ch:0 -> UnitGesture
        M5_LOGE("Failed to add children");
        m5::unit::wiring::failStop();
    }

    // Board-aware I2C for the PaHub: NessoN1 -> PortB GROVE (SoftwareI2C), NanoC6/NanoH2 -> Ex_I2C,
    // others -> Wire. The UnitGesture is reached through the hub (added above), so only the hub is added here.
    if (!m5::unit::wiring::addI2C(Units, hub) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        m5::unit::wiring::failStop();
    }

    M5_LOGI("M5UnitUnified initialized");
    M5_LOGI("%s", Units.debugInfo().c_str());

    // A board without a screen reports a 1x1 dummy display
    has_lcd = (lcd.width() > 8 && lcd.height() > 8);
    if (has_lcd) {
        draw_gesture("Gesture");
    }
}

void loop()
{
    M5.update();
    Units.update();

    if (unit.updated()) {
        const auto g = unit.gesture();
        if (g != gesture_t::None) {
            M5.Log.printf("Gesture:%s\n", gesture_to_string(g));
            if (has_lcd) {
                draw_gesture(gesture_to_string(g));
            }
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
