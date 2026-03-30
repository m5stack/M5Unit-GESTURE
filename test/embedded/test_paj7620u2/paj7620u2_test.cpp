/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for UnitPAJ7620U2
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <googletest/test_helper.hpp>
#include <unit/unit_PAJ7620U2.hpp>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::paj7620u2;
using namespace m5::unit::paj7620u2::command;

struct TestParams {
    bool store_on_change;
};

class TestPAJ7620U2 : public I2CComponentTestBase<UnitPAJ7620U2>, public ::testing::WithParamInterface<TestParams> {
protected:
    virtual UnitPAJ7620U2* get_instance() override
    {
        auto ptr = new m5::unit::UnitPAJ7620U2();
        if (ptr) {
            auto ccfg        = ptr->component_config();
            ccfg.stored_size = 8;
            ptr->component_config(ccfg);

            auto cfg            = ptr->config();
            cfg.start_periodic  = false;
            cfg.store_on_change = GetParam().store_on_change;
            ptr->config(cfg);
        }
        return ptr;
    }
};

INSTANTIATE_TEST_SUITE_P(ParamValues, TestPAJ7620U2, ::testing::Values(TestParams{true}, TestParams{false}));

using check_param_callback = void (*)(UnitPAJ7620U2*);

TEST_P(TestPAJ7620U2, Suspend)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->suspend());
    EXPECT_TRUE(unit->resume());

    // Verify sensor works after resume
    Gesture ges{};
    EXPECT_TRUE(unit->readGesture(ges));
}

TEST_P(TestPAJ7620U2, Gesture)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->writeMode(Mode::Gesture));

    EXPECT_TRUE(unit->writeFrequency(Frequency::Gaming));
    EXPECT_EQ(unit->frequency(), Frequency::Gaming);
    EXPECT_TRUE(unit->writeFrequency(Frequency::Normal));
    EXPECT_EQ(unit->frequency(), Frequency::Normal);

    Gesture ges{};
    uint16_t size{}, x{}, y{};
    uint8_t noobj{}, nomot{};
    EXPECT_TRUE(unit->readGesture(ges));
    EXPECT_TRUE(unit->readObjectSize(size));
    EXPECT_TRUE(unit->readObjectCenter(x, y));
    EXPECT_TRUE(unit->readNoObjectCount(noobj));
    EXPECT_TRUE(unit->readNoMotionCount(nomot));

    unit->update();
    EXPECT_EQ(unit->brightness(), 0);
    EXPECT_FALSE(unit->approach());
    EXPECT_EQ(unit->cursorX(), 0xFFFF);
    EXPECT_EQ(unit->cursorY(), 0xFFFF);

    //
    EXPECT_FALSE(unit->inPeriodic());
    EXPECT_TRUE(unit->startPeriodicMeasurement(10));
    EXPECT_TRUE(unit->inPeriodic());
    EXPECT_EQ(unit->interval(), 10U);

    auto r = collect_periodic_measurements(unit.get(), 16, 0, check_param_callback(nullptr));
    // store_on_change=true: data doesn't change without actual gesture, so timeout is expected
    if (!GetParam().store_on_change) {
        EXPECT_FALSE(r.timed_out);
        EXPECT_EQ(r.update_count, 16U);
        EXPECT_LE(r.median(), r.expected_interval + 5);
    }

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());
    EXPECT_EQ(unit->cursorX(), 0xFFFF);
    EXPECT_EQ(unit->cursorY(), 0xFFFF);

    uint32_t cnt{};

    // They do not accumulate in the same state
    if (GetParam().store_on_change) {
        EXPECT_FALSE(unit->full());
        EXPECT_FALSE(unit->empty());
        EXPECT_EQ(unit->available(), 1);

        while (unit->available()) {
            ++cnt;
            unit->discard();
        }
        EXPECT_EQ(cnt, 1);
        EXPECT_TRUE(unit->empty());
        EXPECT_FALSE(unit->full());
        EXPECT_EQ(unit->available(), 0U);

    } else {
        EXPECT_TRUE(unit->full());
        EXPECT_FALSE(unit->empty());
        EXPECT_EQ(unit->available(), 8);

        while (unit->available()) {
            ++cnt;
            unit->discard();
            EXPECT_EQ(unit->available(), 8U - cnt);
        }
        EXPECT_EQ(cnt, 8);
        EXPECT_TRUE(unit->empty());
        EXPECT_FALSE(unit->full());
        EXPECT_EQ(unit->available(), 0U);
    }
}

TEST_P(TestPAJ7620U2, Proximity)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->writeMode(Mode::Proximity));

    Gesture ges{};
    uint8_t brightness{}, approach{};
    EXPECT_TRUE(unit->readGesture(ges));
    EXPECT_TRUE(unit->readProximity(brightness, approach));

    EXPECT_TRUE(unit->writeApproachThreshold(98, 76));
    uint8_t high{}, low{};
    EXPECT_TRUE(unit->readApproachThreshold(high, low));
    EXPECT_EQ(high, 98);
    EXPECT_EQ(low, 76);

    unit->update();
    EXPECT_EQ(unit->cursorX(), 0xFFFF);
    EXPECT_EQ(unit->cursorY(), 0xFFFF);
}

TEST_P(TestPAJ7620U2, Cursor)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->writeMode(Mode::Cursor));

    uint16_t x{}, y{};
    EXPECT_TRUE(unit->readCursor(x, y));

    unit->update();
    EXPECT_EQ(unit->brightness(), 0);
    EXPECT_FALSE(unit->approach());
}

TEST_P(TestPAJ7620U2, Flip)
{
    SCOPED_TRACE(ustr);

    bool flip{}, flip2{};

    EXPECT_TRUE(unit->readHorizontalFlip(flip));
    EXPECT_TRUE(unit->writeHorizontalFlip(!flip));
    EXPECT_TRUE(unit->readHorizontalFlip(flip2));
    EXPECT_NE(flip, flip2);

    EXPECT_TRUE(unit->readVerticalFlip(flip));
    EXPECT_TRUE(unit->writeVerticalFlip(!flip));
    EXPECT_TRUE(unit->readVerticalFlip(flip2));
    EXPECT_NE(flip, flip2);
}

TEST_P(TestPAJ7620U2, ProximityPeriodic)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->writeMode(Mode::Proximity));
    EXPECT_TRUE(unit->startPeriodicMeasurement(10));
    EXPECT_TRUE(unit->inPeriodic());

    auto r = collect_periodic_measurements(unit.get(), 16, 0, check_param_callback(nullptr));
    if (!GetParam().store_on_change) {
        EXPECT_FALSE(r.timed_out);
        EXPECT_EQ(r.update_count, 16U);
        EXPECT_LE(r.median(), r.expected_interval + 5);
    }

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());
}

TEST_P(TestPAJ7620U2, StartPeriodicWithModeAndFreq)
{
    SCOPED_TRACE(ustr);

    EXPECT_FALSE(unit->inPeriodic());
    EXPECT_TRUE(unit->startPeriodicMeasurement(Mode::Proximity, Frequency::Normal, 10));
    EXPECT_TRUE(unit->inPeriodic());
    EXPECT_EQ(unit->mode(), Mode::Proximity);

    EXPECT_TRUE(unit->stopPeriodicMeasurement());
    EXPECT_FALSE(unit->inPeriodic());
}

TEST_P(TestPAJ7620U2, ExistsObject)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->writeMode(Mode::Gesture));

    bool exists{};
    EXPECT_TRUE(unit->existsObject(exists));
}

TEST_P(TestPAJ7620U2, ReadFrequency)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->writeMode(Mode::Gesture));

    EXPECT_TRUE(unit->writeFrequency(Frequency::Normal));
    Frequency f{};
    EXPECT_TRUE(unit->readFrequency(f));
    EXPECT_EQ(f, Frequency::Normal);

    EXPECT_TRUE(unit->writeFrequency(Frequency::Gaming));
    EXPECT_TRUE(unit->readFrequency(f));
    EXPECT_EQ(f, Frequency::Gaming);

    uint16_t raw{};
    EXPECT_TRUE(unit->readFrequency(raw));
}

TEST_P(TestPAJ7620U2, FrequencyHz)
{
    SCOPED_TRACE(ustr);

    // Write 120Hz and read back
    EXPECT_TRUE(unit->writeFrequencyHz(120.0f));
    float hz = unit->readFrequencyHz();
    EXPECT_NEAR(hz, 120.0f, 1.0f);
    // Should match Normal preset
    EXPECT_EQ(unit->frequency(), Frequency::Normal);

    // Write 240Hz and read back
    EXPECT_TRUE(unit->writeFrequencyHz(240.0f));
    hz = unit->readFrequencyHz();
    EXPECT_NEAR(hz, 240.0f, 1.0f);
    // Should match Gaming preset
    EXPECT_EQ(unit->frequency(), Frequency::Gaming);

    // Write arbitrary Hz (60Hz)
    EXPECT_TRUE(unit->writeFrequencyHz(60.0f));
    hz = unit->readFrequencyHz();
    EXPECT_NEAR(hz, 60.0f, 1.0f);
    // Not a preset
    EXPECT_EQ(unit->frequency(), Frequency::Unknown);

    // Restore to Normal
    EXPECT_TRUE(unit->writeFrequency(Frequency::Normal));
}

TEST_P(TestPAJ7620U2, Rotation)
{
    SCOPED_TRACE(ustr);

    for (uint8_t r = 0; r < 4; ++r) {
        unit->setRotate(r);
        EXPECT_EQ(unit->rotation(), r);
    }
    // Wraps at 4
    unit->setRotate(4);
    EXPECT_EQ(unit->rotation(), 0);
}

TEST_P(TestPAJ7620U2, EnableDisable)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->disable());
    EXPECT_TRUE(unit->enable());

    // Verify sensor works after re-enable
    Gesture ges{};
    EXPECT_TRUE(unit->readGesture(ges));
}

// Test that begin() applies config_t values
struct BeginConfigParams {
    Mode mode;
    Frequency frequency;
    bool hflip;
    bool vflip;
    uint8_t rotation;
};

class TestPAJ7620U2BeginConfig : public I2CComponentTestBase<UnitPAJ7620U2>,
                                 public ::testing::WithParamInterface<BeginConfigParams> {
protected:
    virtual UnitPAJ7620U2* get_instance() override
    {
        auto ptr = new m5::unit::UnitPAJ7620U2();
        if (ptr) {
            auto ccfg        = ptr->component_config();
            ccfg.stored_size = 8;
            ptr->component_config(ccfg);

            auto cfg           = ptr->config();
            cfg.start_periodic = false;
            cfg.mode           = GetParam().mode;
            cfg.frequency      = GetParam().frequency;
            cfg.hflip          = GetParam().hflip;
            cfg.vflip          = GetParam().vflip;
            cfg.rotation       = GetParam().rotation;
            ptr->config(cfg);
        }
        return ptr;
    }
};

INSTANTIATE_TEST_SUITE_P(ConfigValues, TestPAJ7620U2BeginConfig,
                         ::testing::Values(
                             // Default config
                             BeginConfigParams{Mode::Gesture, Frequency::Normal, false, true, 0},
                             // Non-default: Proximity, Gaming, flips inverted, rotation 2
                             BeginConfigParams{Mode::Proximity, Frequency::Gaming, true, false, 2}));

TEST_P(TestPAJ7620U2BeginConfig, BeginAppliesConfig)
{
    SCOPED_TRACE(ustr);
    const auto& p = GetParam();

    // Mode (memory)
    EXPECT_EQ(unit->mode(), p.mode);

    // Frequency (memory + register)
    EXPECT_EQ(unit->frequency(), p.frequency);
    if (p.mode != Mode::Proximity) {
        Frequency f{};
        EXPECT_TRUE(unit->readFrequency(f));
        EXPECT_EQ(f, p.frequency);
    } else {
        // Proximity mode table overwrites R_IDLE_TIME; enum read returns Unknown
        Frequency f{};
        EXPECT_FALSE(unit->readFrequency(f));
        // But raw read succeeds and Hz gives ~10Hz
        float hz = unit->readFrequencyHz();
        EXPECT_NEAR(hz, 10.0f, 1.0f);
    }

    // Flip (register)
    bool h{}, v{};
    EXPECT_TRUE(unit->readHorizontalFlip(h));
    EXPECT_TRUE(unit->readVerticalFlip(v));
    EXPECT_EQ(h, p.hflip);
    EXPECT_EQ(v, p.vflip);

    // Rotation (memory only, not stored in register)
    EXPECT_EQ(unit->rotation(), p.rotation);
}
