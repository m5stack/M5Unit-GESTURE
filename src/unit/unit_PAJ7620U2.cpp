/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_PAJ7620U2.cpp
  @brief PAJ7620U2Unit for M5UnitUnified
*/
#include "unit_PAJ7620U2.hpp"
#include <M5Utility.hpp>
#include <array>
#include <cstring>
#include <driver/gpio.h>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit::paj7620u2;
using namespace m5::unit::paj7620u2::command;

namespace {
constexpr uint16_t chip_id{0x7620};
constexpr uint8_t wakeup_value{0x20};
constexpr uint8_t enter_suspend{0x01};

#if 0
// Hz to IDLE_TIME
int freq_to_idle(const float frequency) {
    return static_cast<int>((1000.0f / frequency - 3.55f) / 0.0323f);
}
// IDLE_TIME to Hz
float idle_to_freq(const int idleTime) {
    return 1000.0f / (idleTime * 0.0323f + 3.55f);
}
#endif

struct Pair {
    uint8_t reg, val;
};

// initialize parameter
constexpr Pair register_for_initialize[] = {
#if defined(USING_REGISTER_VALUE_15)
    // Datasheet V1.5 (2022-01-05) register values
    {0xEF, 0x00},                                                          // Bank 0
    {0x41, 0xFF},                                                          // R_Int_1_En [7:0]
    {0x42, 0x01},                                                          // R_Int_2_En [7:0]
    {0x46, 0x2D},                                                          // R_AELedOff_UB [7:0]
    {0x47, 0x0F},                                                          // R_AELedOff_LB [7:0]
    {0x48, 0x80},                                                          // R_AE_Exposure_UB [7:0]
    {0x49, 0x00},                                                          // R_AE_Exposure_UB [15:8]
    {0x4A, 0x40},                                                          // R_AE_Exposure_LB [7:0]
    {0x4B, 0x00},                                                          // R_AE_Exposure_LB [15:8]
    {0x4C, 0x20},                                                          // R_AE_Gain_UB [7:0]
    {0x4D, 0x00},                                                          // R_AE_Gain_LB [7:0]
    {0x51, 0x10},                                                          // R_Manual_GG[0]
    {0x5C, 0x02}, {0x5E, 0x10},                                            // TG___CLK_manual
    {0x80, 0x41},                                                          // Im_GPIO0
    {0x81, 0x44},                                                          // Tm_GPIO2_OEL
    {0x82, 0x0C},                                                          // Im_INT
    {0x83, 0x20},                                                          // R_LightThd [7:0]
    {0x84, 0x20}, {0x85, 0x00}, {0x86, 0x10}, {0x87, 0x00}, {0x8B, 0x01},  // R_Cursor_ObjectSizeTh [7:0]
    {0x8D, 0x00}, {0x90, 0x0C},                                            // R_NoMotionCountThd [6:0]
    {0x91, 0x0C}, {0x93, 0x0D}, {0x94, 0x0A}, {0x95, 0x0A},                // R_ZDirectionThd [4:0]
    {0x96, 0x0C},                                                          // R_ZDirectionXYThd [4:0]
    {0x97, 0x05},                                                          // R_ZDirectionAngleThd [3:0]
    {0x9A, 0x14},                                                          // R_RotateXYThd [4:0]
    {0x9C, 0x3F},                                                          // R_FilterWeight [1:0], FilterDistThd [6:2]
    {0x9F, 0xF9},                                                          // R_RotateEnH
    {0xA0, 0x48}, {0xA5, 0x19},  // R_FilterImage [0], R_FilterAverage_Mode [3:2]
    {0xCC, 0x19},                // R_YtoZSum[5:0]
    {0xCD, 0x0B},                // R_YtoZFactor[5:0]
    {0xCE, 0x13},                // R_PositionFilterLength[2:0],R_ProcessFilterLength[6:4]
    {0xCF, 0x62},                // R_WaveCountThd[3:0],R_WaveAngleThd[7:4]
    {0xD0, 0x21},                // R_AbortCountThd[2:0],R_AbortXYRatio[7:3]
    {0xEF, 0x01},                // Bank 1
    {0x00, 0x1E},                // Cmd_HSize [5:0]
    {0x01, 0x1E},                // Cmd_VSize [5:0]
    {0x02, 0x0F},                // Cmd_HStart [5:0]
    {0x03, 0x0F},                // Cmd_VStart [5:0]
    {0x04, 0x02},                // R_HR_LS_Comp_DAvg_V
    {0x25, 0x01},                // R_LensShadingComp_EnH [0]
    {0x26, 0x00}, {0x27, 0x39},  // R_OffsetY [6:0]
    {0x28, 0x7F},                // R_LSC [6:0]
    {0x29, 0x08},                // R_LSFT [3:0]
    {0x30, 0x03}, {0x3E, 0xFF},  // R_DebugPattern[7:0]
    {0x5E, 0x3D},                // T_clamp_drv_ctrl
    {0x65, 0xAC},                // R_IDLE_TIME [7:0] (~110 Hz)
    {0x66, 0x00},                // R_IDLE_TIME [15:8]
    {0x67, 0x97},                // R_IDLE_TIME_SLEEP_1 [7:0]
    {0x68, 0x01},                // R_IDLE_TIME_SLEEP_1 [15:8]
    {0x69, 0xCD},                // R_IDLE_TIME_SLEEP_2 [7:0]
    {0x6A, 0x01},                // R_IDLE_TIME_SLEEP_2 [15:8]
    {0x6B, 0xB0},                // R_Obj_TIME_1 [7:0]
    {0x6C, 0x04},                // R_Obj_TIME_1 [15:8]
    {0x6D, 0x2C},                // R_Obj_TIME_2 [7:0]
    {0x6E, 0x01},                // R_Obj_TIME_2 [15:8]
    {0x72, 0x01},                // R_TG_EnH Enable/Disable PAJ7620U2[0]
    {0x73, 0x35},                // R_AUTO_SLEEP_Mode
    {0x74, 0x00},                // R_WakeUpSig_Sel 0:gesture
    {0x77, 0x01},                // R_SRAM_Read_EnH[0]
    {0xEF, 0x00},                // Bank 0
#else
    // Datasheet V0.7 (2014-05-22) register values
    {0xEF, 0x00},  // Bank 0
    {0x41, 0x00},  // R_Int_1_En [7:0] (disable first)
    {0x42, 0x00},  // R_Int_2_En [7:0] (disable first)
    {0x37, 0x07},  // R_CursorClampLeft [4:0]
    {0x38, 0x17},  // R_CursorClampRight [4:0]
    {0x39, 0x06},  // R_CursorClampUp [4:0]
    {0x42, 0x01},  // R_Int_2_En [7:0]
    {0x46, 0x2D},  // R_AELedOff_UB [7:0]
    {0x47, 0x0F},  // R_AELedOff_LB [7:0]
    {0x48, 0x3C},  // R_AE_Exposure_UB [7:0]
    {0x49, 0x00},  // R_AE_Exposure_UB [15:8]
    {0x4A, 0x1E},  // R_AE_Exposure_LB [7:0]
    {0x4C, 0x20},  // R_AE_Gain_UB [7:0] (was 0x22, fixed to match datasheet)
    {0x51, 0x10},  // R_Manual_GG[0]
    {0x5E, 0x10},  // TG___CLK_manual
    {0x60, 0x27},  // TS_osc_code[6:0],OSC_BIST_OK[7]
    {0x80, 0x42},  // Im_GPIO0
    {0x81, 0x44},  // Tm_GPIO2_OEL
    {0x82, 0x04},  // Im_INT
    {0x8B, 0x01},  // R_Cursor_ObjectSizeTh [7:0]
    {0x90, 0x06},  // R_NoMotionCountThd [6:0]
    {0x95, 0x0A},  // R_ZDirectionThd [4:0]
    {0x96, 0x0C},  // R_ZDirectionXYThd [4:0]
    {0x97, 0x05},  // R_ZDirectionAngleThd [3:0]
    {0x9A, 0x14},  // R_RotateXYThd [4:0]
    {0x9C, 0x3F},  // R_FilterWeight [1:0], FilterDistThd [6:2]
    {0xA5, 0x19},  // R_FilterImage [0], R_FilterAverage_Mode [3:2]
    {0xCC, 0x19},  // R_YtoZSum[5:0]
    {0xCD, 0x0B},  // R_YtoZFactor[5:0]
    {0xCE, 0x13},  // R_PositionFilterLength[2:0],R_ProcessFilterLength[6:4]
    {0xCF, 0x64},  // R_WaveCountThd[3:0],R_WaveAngleThd[7:4]
    {0xD0, 0x21},  // R_AbortCountThd[2:0],R_AbortXYRatio[7:3]
    {0xEF, 0x01},  // Bank 1
    {0x02, 0x0F},  // Cmd_HStart [5:0]
    {0x03, 0x10},  // Cmd_VStart [5:0]
    {0x04, 0x02},  // R_HR_LS_Comp_DAvg_V
    {0x25, 0x01},  // R_LensShadingComp_EnH [0]
    {0x27, 0x39},  // R_OffsetY [6:0]
    {0x28, 0x7F},  // R_LSC [6:0]
    {0x29, 0x08},  // R_LSFT [3:0]
    {0x3E, 0xFF},  // R_DebugPattern[7:0]
    {0x5E, 0x3D},  // T_clamp_drv_ctrl
    {0x65, 0x96},  // R_IDLE_TIME [7:0] (~120 Hz)
    {0x67, 0x97},  // R_IDLE_TIME_SLEEP_1 [7:0]
    {0x69, 0xCD},  // R_IDLE_TIME_SLEEP_2 [7:0]
    {0x6A, 0x01},  // R_IDLE_TIME_SLEEP_2 [15:8]
    {0x6D, 0x2C},  // R_Obj_TIME_2 [7:0]
    {0x6E, 0x01},  // R_Obj_TIME_2 [15:8]
    {0x72, 0x01},  // R_TG_EnH Enable/Disable PAJ7620U2[0]
    {0x73, 0x35},  // R_AUTO_SLEEP_Mode
    {0x74, 0x00},  // R_WakeUpSig_Sel 0:gesture
    {0x77, 0x01},  // R_SRAM_Read_EnH[0]
    {0xEF, 0x00},  // Bank 0
    {0x41, 0xFF},  // Re-enable interrupts for first 8 gestures
    {0x42, 0x01},  // Re-enable interrupts for wave gesture
#endif
};

// gesture mode
constexpr Pair register_for_gesture[] = {
#if defined(USING_REGISTER_VALUE_15)
    // Datasheet V1.5 (2022-01-05) register values
    {0xEF, 0x00},  // Bank 0
    {0x41, 0x00},  // R_Int_1_En [7:0]
    {0x42, 0x00},  // R_Int_2_En [7:0]
    {0x48, 0x3C},  // R_AE_Exposure_UB [7:0]
    {0x49, 0x00},  // R_AE_Exposure_UB [15:8]
    {0x51, 0x10},  // R_Manual_GG[0]
    {0x83, 0x20},  // R_LightThd [7:0]
    {0x9F, 0xF9},  // R_RotateEnH
    {0xEF, 0x01},  // Bank 1
    {0x01, 0x1E},  // Cmd_VSize [5:0]
    {0x02, 0x0F},  // Cmd_HStart [5:0]
    {0x03, 0x0F},  // Cmd_VStart [5:0]
    {0x04, 0x02},  // R_HR_LS_Comp_DAvg_V
    {0x41, 0x40},
    {0x43, 0x30},
    {0x65, 0xAC},  // R_IDLE_TIME [7:0] (~110 Hz)
    {0x66, 0x00},  // R_IDLE_TIME [15:8]
    {0x67, 0x97},  // R_IDLE_TIME_SLEEP_1 [7:0]
    {0x68, 0x01},  // R_IDLE_TIME_SLEEP_1 [15:8]
    {0x69, 0xCD},  // R_IDLE_TIME_SLEEP_2 [7:0]
    {0x6A, 0x01},  // R_IDLE_TIME_SLEEP_2 [15:8]
    {0x6B, 0xB0},  // R_Obj_TIME_1 [7:0]
    {0x6C, 0x04},  // R_Obj_TIME_1 [15:8]
    {0x6D, 0x2C},  // R_Obj_TIME_2 [7:0]
    {0x6E, 0x01},  // R_Obj_TIME_2 [15:8]
    {0x74, 0x00},  // R_WakeUpSig_Sel 0:gesture
    {0xEF, 0x00},  // Bank 0
    {0x41, 0xFF},  // R_Int_1_En [7:0]
    {0x42, 0x01},  // R_Int_2_En [7:0]
#else
    // Datasheet V0.7 (2014-05-22) register values
    {0xEF, 0x00},  // Bank 0
    {0x41, 0x00},  // R_Int_1_En [7:0]
    {0x42, 0x00},  // R_Int_2_En [7:0]
    {0x48, 0x3C},  // R_AE_Exposure_UB [7:0]
    {0x49, 0x00},  // R_AE_Exposure_UB [15:8]
    {0x51, 0x10},  // R_Manual_GG[0]
    {0x83, 0x20},  // R_LightThd [7:0]
    {0x9F, 0xF9},  // R_RotateEnH
    {0xEF, 0x01},  // Bank 1
    {0x01, 0x1E},  // Cmd_VSize [5:0]
    {0x02, 0x0F},  // Cmd_HStart [5:0]
    {0x03, 0x10},  // Cmd_VStart [5:0]
    {0x04, 0x02},  // R_HR_LS_Comp_DAvg_V
    {0x41, 0x40},
    {0x43, 0x30},
    {0x65, 0x96},  // R_IDLE_TIME [7:0] (~120 Hz)
    {0x66, 0x00},  // R_IDLE_TIME [15:8]
    {0x67, 0x97},  // R_IDLE_TIME_SLEEP_1 [7:0]
    {0x68, 0x01},  // R_IDLE_TIME_SLEEP_1 [15:8]
    {0x69, 0xCD},  // R_IDLE_TIME_SLEEP_2 [7:0]
    {0x6A, 0x01},  // R_IDLE_TIME_SLEEP_2 [15:8]
    {0x6B, 0xB0},  // R_Obj_TIME_1 [7:0]
    {0x6C, 0x04},  // R_Obj_TIME_1 [15:8]
    {0x6D, 0x2C},  // R_Obj_TIME_2 [7:0]
    {0x6E, 0x01},  // R_Obj_TIME_2 [15:8]
    {0x74, 0x00},  // R_WakeUpSig_Sel 0:gesture
    {0xEF, 0x00},  // Bank 0
    {0x41, 0xFF},  // R_Int_1_En [7:0]
    {0x42, 0x01},  // R_Int_2_En [7:0]
#endif
    {0xFF /*terminator*/, 0xFF}};
// proximity mode
constexpr Pair register_for_proximity[] = {
#if defined(USING_REGISTER_VALUE_15)
    // Datasheet V1.5 (2022-01-05) register values
    {0xEF, 0x00},  // Bank 0
    {0x41, 0x00},  // R_Int_1_En [7:0]
    {0x42, 0x02},  // R_Int_2_En [7:0]
    {0x48, 0x20},  // R_AE_Exposure_UB [7:0]
    {0x49, 0x00},  // R_AE_Exposure_UB [15:8]
    {0x51, 0x13},  // R_Manual_GG[0]
    {0x83, 0x00},  // R_LightThd [7:0]
    {0x9F, 0xF8},  // R_RotateEnH
    {0x69, 0x96},  // R_Pox_UB [7:0]
    {0x6A, 0x02},  // R_Pox_LB [7:0]
    {0xEF, 0x01},  // Bank 1
    {0x01, 0x1E},  // Cmd_VSize [5:0]
    {0x02, 0x0F},  // Cmd_HStart [5:0]
    {0x03, 0x0F},  // Cmd_VStart [5:0]
    {0x04, 0x02},  // R_HR_LS_Comp_DAvg_V
    {0x41, 0x50},  // R_dac_ctrl
    {0x43, 0x34},  // R_pga_test
    {0x65, 0xCE},  // R_IDLE_TIME [7:0]
    {0x66, 0x0B},  // R_IDLE_TIME [15:8]
    {0x67, 0xCE},  // R_IDLE_TIME_SLEEP_1 [7:0]
    {0x68, 0x0B},  // R_IDLE_TIME_SLEEP_1 [15:8]
    {0x69, 0xE9},  // R_IDLE_TIME_SLEEP_2 [7:0]
    {0x6A, 0x05},  // R_IDLE_TIME_SLEEP_2 [15:8]
    {0x6B, 0x50},  // R_Obj_TIME_1 [7:0]
    {0x6C, 0xC3},  // R_Obj_TIME_1 [15:8]
    {0x6D, 0x50},  // R_Obj_TIME_2 [7:0]
    {0x6E, 0xC3},  // R_Obj_TIME_2 [15:8]
    {0x74, 0x05},  // R_WakeUpSig_SelEnable 5:proximity
#else
    // Datasheet V0.7 (2014-05-22) register values
    {0xEF, 0x00},  // Bank 0
    {0x41, 0x00},  // R_Int_1_En [7:0]
    {0x42, 0x02},  // R_Int_2_En [7:0]
    {0x48, 0x20},  // R_AE_Exposure_UB [7:0]
    {0x49, 0x00},  // R_AE_Exposure_UB [15:8]
    {0x51, 0x13},  // R_Manual_GG[0]
    {0x83, 0x00},  // R_LightThd [7:0]
    {0x9F, 0xF8},  // R_RotateEnH
    {0x69, 0x96},  // R_Pox_UB [7:0]
    {0x6A, 0x02},  // R_Pox_LB [7:0]
    {0xEF, 0x01},  // Bank 1
    {0x01, 0x1E},  // Cmd_VSize [5:0]
    {0x02, 0x0F},  // Cmd_HStart [5:0]
    {0x03, 0x10},  // Cmd_VStart [5:0]
    {0x04, 0x02},  // R_HR_LS_Comp_DAvg_V
    {0x41, 0x50},  // R_dac_ctrl
    {0x43, 0x34},  // R_pga_test
    {0x65, 0xCE},  // R_IDLE_TIME [7:0]
    {0x66, 0x0B},  // R_IDLE_TIME [15:8]
    {0x67, 0xCE},  // R_IDLE_TIME_SLEEP_1 [7:0]
    {0x68, 0x0B},  // R_IDLE_TIME_SLEEP_1 [15:8]
    {0x69, 0xE9},  // R_IDLE_TIME_SLEEP_2 [7:0]
    {0x6A, 0x05},  // R_IDLE_TIME_SLEEP_2 [15:8]
    {0x6B, 0x50},  // R_Obj_TIME_1 [7:0]
    {0x6C, 0xC3},  // R_Obj_TIME_1 [15:8]
    {0x6D, 0x50},  // R_Obj_TIME_2 [7:0]
    {0x6E, 0xC3},  // R_Obj_TIME_2 [15:8]
    {0x74, 0x05},  // R_WakeUpSig_SelEnable 5:proximity
#endif
    {0xFF /*terminator*/, 0xFF}};
// cursor mode
constexpr Pair register_for_cursor[] = {
    // restore
    {0xEF, 0x00},
    {0x48, 0x3C},
    {0x49, 0x00},
    {0x51, 0x10},
    {0x83, 0x20},
    {0x9F, 0xF9},
    {0x69, 0x14},
    {0x6A, 0x0A},
    {0xEF, 0x00},  // Set Bank 0
    {0x32, 0x29},  // R_CursorClampLeft
    {0x33, 0x01},  // R_PositionFilterStartSizeTh [7:0]
    {0x34, 0x00},  // R_PositionFilterStartSizeTh [8]
    {0x35, 0x01},  // R_ProcessFilterStartSizeTh [7:0]
    {0x36, 0x00},  // R_ProcessFilterStartSizeTh [8]
    {0x37, 0x03},  // R_CursorClampLeft [4:0]
    {0x38, 0x1B},  // R_CursorClampRight [4:0]
    {0x39, 0x03},  // R_CursorClampUp [4:0]
    {0x3A, 0x1B},  // R_CursorClampDown [4:0]
    {0x41, 0x00},  // R_Int_1_En [7:0]
    {0x42, 0x84},  // R_Int_2_En [7:0]
    {0x8B, 0x01},  // R_Cursor_ObjectSizeTh [7:0]
    {0x8C, 0x07},  // R_PositionResolution [2:0]
    {0xEF, 0x01},  // Bank 1
    {0x04, 0x03},  // R_HR_LS_Comp_DAvg_V (XY flip)
    {0x74, 0x03},  // R_WakeUpSig_SelEnable 3:cursor
    {0xFF /*terminator*/, 0xFF}};

constexpr const Pair* register_table[] = {
    register_for_gesture,
    register_for_proximity,
    register_for_cursor,
};

constexpr Gesture rotate_1[] = {
    Gesture::Left,
    Gesture::Right,
    Gesture::Down,
    Gesture::Up,

};
constexpr Gesture rotate_2[] = {
    Gesture::Down,
    Gesture::Up,
    Gesture::Right,
    Gesture::Left,
};
constexpr Gesture rotate_3[] = {
    Gesture::Right,
    Gesture::Left,
    Gesture::Up,
    Gesture::Down,
};
// CCW
constexpr const Gesture* rotate_table[] = {
    nullptr,
    rotate_1,
    rotate_2,
    rotate_3,
};

Gesture rotate_gesture(const Gesture g, const uint8_t rot)
{
    auto gv = m5::stl::to_underlying(g);
    auto p  = rotate_table[rot & 0x03];
    if (!p || (gv & 0x0F) == 0) {
        return g;
    }
    return static_cast<Gesture>(p[__builtin_ctz(gv)]);
}

constexpr uint8_t freq_table[] = {
#if defined(USING_REGISTER_VALUE_15)
    0xAC,  // Normal  ~110Hz (V1.5)
    0x13,  // Gaming  ~240Hz
#else
    0x96,  // Normal  ~120Hz (V0.7)
    0x13,  // Gaming  ~240Hz
#endif
};

}  // namespace

namespace m5 {
namespace unit {
// class UnitPAJ7620U2
const char UnitPAJ7620U2::name[] = "UnitPAJ7620U2";
const types::uid_t UnitPAJ7620U2::uid{"UnitPAJ7620U2"_mmh3};
const types::attr_t UnitPAJ7620U2::attr{attribute::AccessI2C};

bool UnitPAJ7620U2::begin()
{
    auto ssize = stored_size();
    assert(ssize && "stored_size must be greater than zero");
    if (ssize != _data->capacity()) {
        _data.reset(new m5::container::CircularBuffer<Data>(ssize));
        if (!_data) {
            M5_LIB_LOGE("Failed to allocate");
            return false;
        }
    }

    uint16_t id{};
    uint8_t ver{};

    if (!wakeup()) {
        return false;
    }

    // Check chip ID and get version
    if (!read_chip_id(id) || !read_version(ver)) {
        M5_LIB_LOGE("Failed to get id/version %x:%x", id, ver);
        return false;
    }
    if (id != chip_id) {
        M5_LIB_LOGE("Not PAJ7620U2 %x", id);
        return false;
    }

    // rotation
    _rotation = _cfg.rotation & 0x03;

    // Set initialize value to registers
    for (auto&& e : register_for_initialize) {
        if (!writeRegister8(e.reg, e.val)) {
            M5_LIB_LOGE("Failed to initialize [%02x]:%x", e.reg, e.val);
            return false;
        }
    }
    if (!select_bank(0, true) || !writeFrequency(_cfg.frequency) || !writeMode(_cfg.mode)) {
        M5_LIB_LOGE("Failed to settings");
        return false;
    }

    return _cfg.start_periodic ? startPeriodicMeasurement() : true;
}

void UnitPAJ7620U2::update(const bool force)
{
    _updated = false;
    if (inPeriodic()) {
        elapsed_time_t at{m5::utility::millis()};
        if (force || !_latest || at >= _latest + _interval) {
            Data d{};
            switch (_mode) {
                case Mode::Gesture:
                    _updated = update_gesture(d);
                    if (_updated && _cfg.store_on_change && !empty()) {
                        _updated = latest().gesture() != d.gesture();
                    }
                    break;
                case Mode::Proximity:
                    _updated = update_proximity(d);
                    if (_updated && _cfg.store_on_change && !empty()) {
                        auto ld  = latest();
                        _updated = ld.gesture() != d.gesture() || ld.brightness() != d.brightness() ||
                                   ld.approach() != d.approach();
                    }
                    break;
                case Mode::Cursor:
                    _updated = update_cursor(d);
                    if (_updated && _cfg.store_on_change && !empty()) {
                        auto ld = latest();
                        _updated =
                            ld.gesture() != d.gesture() || ld.cursorX() != d.cursorX() || ld.cursorY() != d.cursorY();
                    }
                    break;
                default:
                    return;
            }
            if (_updated) {
                _latest = at;
                _data->push_back(d);
            }
        }
    }
}

bool UnitPAJ7620U2::update_gesture(paj7620u2::Data& d)
{
    if (read_gesture(d)) {
        d.data_mode = Mode::Gesture;
        uint16_t raw_gesture;
        std::memcpy(&raw_gesture, d.raw.data(), sizeof(raw_gesture));
        d.data_gesture = rotate_gesture(static_cast<Gesture>(raw_gesture), _rotation);
        return true;
    }
    return false;
}

bool UnitPAJ7620U2::update_proximity(paj7620u2::Data& d)
{
    if (read_gesture(d) && read_proximity(d)) {
        d.data_mode            = Mode::Proximity;
        d.proximity_brightness = d.raw[2];
        d.proximity_approach   = d.raw[3];
        return true;
    }
    return false;
}

bool UnitPAJ7620U2::update_cursor(paj7620u2::Data& d)
{
    // if (read_gesture(d) && d.gesture() == Gesture::HasObject && read_cursor(d)) {
    if (read_cursor(d)) {
        d.data_mode = Mode::Cursor;
        d.cursor_x  = (((uint16_t)(d.raw[3] & 0x0F)) << 8) | d.raw[2];
        d.cursor_y  = (((uint16_t)(d.raw[5] & 0x0F)) << 8) | d.raw[4];
        return true;
    }
    //    M5_LIB_LOGE(">>>> %x", d.gesture());
    return false;
}

bool UnitPAJ7620U2::read_gesture(Data& d)
{
    return read_banked_register(INT_FLAG_1, d.raw.data(), 2);
}

bool UnitPAJ7620U2::read_proximity(Data& d)
{
    return read_banked_register(S_AVGY, d.raw.data() + 2, 1) && read_banked_register(S_STATE, d.raw.data() + 3, 1);
}

bool UnitPAJ7620U2::read_cursor(Data& d)
{
    return read_banked_register(OBJECT_CENTER_X_LOW, d.raw.data() + 2, 4);
}

bool UnitPAJ7620U2::readGesture(Gesture& ges)
{
    ges = Gesture::None;
    Data d{};
    if (update_gesture(d)) {
        ges = d.gesture();
        return true;
    }
    return false;
}

bool UnitPAJ7620U2::readNoObjectCount(uint8_t& cnt)
{
    cnt = 0;
    return read_banked_register8(NO_OBJECT_COUNT, cnt);
}

bool UnitPAJ7620U2::readNoMotionCount(uint8_t& cnt)
{
    cnt = 0;
    return read_banked_register8(NO_MOTION_COUNT, cnt);
}

bool UnitPAJ7620U2::readObjectSize(uint16_t& sz)
{
    sz = 0;
    uint8_t buf[2]{};
    if (!read_banked_register(OBJECT_SIZE_LOW, buf, 2)) {
        return false;
    }
    sz = static_cast<uint16_t>(buf[1]) << 8 | buf[0];
    return true;
}

bool UnitPAJ7620U2::readProximity(uint8_t& brightness, uint8_t& approach)
{
    brightness = approach = 0;
    Data d{};
    if (update_proximity(d)) {
        brightness = d.brightness();
        approach   = d.approach();
        return true;
    }
    return false;
}

bool UnitPAJ7620U2::readObjectCenter(uint16_t& x, uint16_t& y)
{
    x = y = 0;
    uint8_t xl{}, xh{}, yl{}, yh{};
    if (read_banked_register8(OBJECT_CENTER_X_LOW, xl) && read_banked_register8(OBJECT_CENTER_X_HIGH, xh) &&
        read_banked_register8(OBJECT_CENTER_Y_LOW, yl) && read_banked_register8(OBJECT_CENTER_Y_HIGH, yh)) {
        x = (((uint16_t)(xh & 0x1F)) << 8) | xl;
        y = (((uint16_t)(yh & 0x1F)) << 8) | yl;
        return true;
    }
    return false;
}

bool UnitPAJ7620U2::readCursor(uint16_t& x, uint16_t& y)
{
    x = y = 0;
    Data d{};
    if (update_cursor(d)) {
        x = d.cursorX();
        y = d.cursorY();
        return true;
    }
    return false;
}

bool UnitPAJ7620U2::enable(const bool flag)
{
    return write_banked_register8(R_TG_ENH, flag ? 1 : 0);
}

bool UnitPAJ7620U2::suspend()
{
    return enable(false) && write_banked_register8(SW_SUSPEND_ENL, enter_suspend);
}

bool UnitPAJ7620U2::resume()
{
    return wakeup() && enable(true);
}

bool UnitPAJ7620U2::readFrequency(uint8_t& raw)
{
    raw = 0;
    return read_banked_register8(R_IDLE_TIME_LOW, raw);
}

bool UnitPAJ7620U2::readFrequency(Frequency& f)
{
    f = Frequency::Unknown;

    uint8_t raw{};
    if (readFrequency(raw)) {
        int8_t idx{};
        for (auto&& e : freq_table) {
            if (e == raw) {
                f = static_cast<Frequency>(idx);
                return true;
            }
            ++idx;
        }
    }
    return false;
}

bool UnitPAJ7620U2::writeFrequency(const Frequency f)
{
    if (f == Frequency::Unknown || !write_banked_register8(R_IDLE_TIME_LOW, freq_table[m5::stl::to_underlying(f)])) {
        return false;
    }
    _frequency = f;
    return true;
}

bool UnitPAJ7620U2::writeMode(const Mode mode)
{
    auto idx       = m5::stl::to_underlying(mode);
    const Pair* rv = idx < m5::stl::size(register_table) ? register_table[idx] : nullptr;
    if (!rv) {
        M5_LIB_LOGE("Invalid mode:%x", mode);
        return false;
    }

    while (rv->reg != 0xFF) {
#if 0
        uint8_t v{};
        if (!readRegister8(rv->reg, v, 0)) {
            return false;
        }
        M5_LIB_LOGE("{0X%02x,0X%02X}", rv->reg, v);
        // M5_LIB_LOGI("[%02X]:%02X", rv->reg, rv->val);
#endif
        if (!writeRegister8(rv->reg, rv->val)) {
            M5_LIB_LOGE("Failed to change mode %x:%x", rv->reg, rv->val);
            return false;
        }
        ++rv;
    }
    _mode = mode;

    // To resolve bank inconsistencies after register setting
    return select_bank(0, true) && ((_mode != Mode::Proximity) ? writeFrequency(_frequency) : true);
}

bool UnitPAJ7620U2::readApproachThreshold(uint8_t& high, uint8_t& low)
{
    return read_banked_register8(R_POX_UB, high) && read_banked_register8(R_POX_LB, low);
}

bool UnitPAJ7620U2::writeApproachThreshold(const uint8_t high, const uint8_t low)
{
    return write_banked_register8(R_POX_UB, high) && write_banked_register8(R_POX_LB, low);
}

bool UnitPAJ7620U2::readHorizontalFlip(bool& flip)
{
    flip = false;
    uint8_t v{};
    if (read_banked_register8(LS_COMP_DAVG_V, v)) {
        flip = (v & 0x01);  // HFlip bit:0
        return true;
    }
    return false;
}

bool UnitPAJ7620U2::readVerticalFlip(bool& flip)
{
    flip = false;
    uint8_t v{};
    if (read_banked_register8(LS_COMP_DAVG_V, v)) {
        flip = (v & 0x02);  // VFlip bit:1
        return true;
    }
    return false;
}

bool UnitPAJ7620U2::writeHorizontalFlip(const bool flip)
{
    uint8_t v{};
    if (read_banked_register8(LS_COMP_DAVG_V, v)) {
        v = (v & ~0x01) | (flip ? 0x01 : 0x00);
        return writeRegister8((uint8_t)(LS_COMP_DAVG_V & 0xFF), v);
    }
    return false;
}

bool UnitPAJ7620U2::writeVerticalFlip(const bool flip)
{
    uint8_t v{};
    if (read_banked_register8(LS_COMP_DAVG_V, v)) {
        v = (v & ~0x02) | (flip ? 0x02 : 0x00);
        return writeRegister8((uint8_t)(LS_COMP_DAVG_V & 0xFF), v);
    }
    return false;
}

//
bool UnitPAJ7620U2::select_bank(const uint8_t bank, const bool force)
{
    if (!force && _current_bank == bank) {
        return true;
    }
    if (writeRegister8(BANK_SEL, bank)) {
        _current_bank = bank;
        return true;
    }
    return false;
}

bool UnitPAJ7620U2::read_banked_register(const uint16_t reg, uint8_t* buf, const size_t len)
{
    return select_bank((reg >> 8) & 1) && readRegister((uint8_t)(reg & 0xFF), buf, len, 1);
}

bool UnitPAJ7620U2::read_banked_register8(const uint16_t reg, uint8_t& value)
{
    return select_bank((reg >> 8) & 1) && readRegister8((uint8_t)(reg & 0xFF), value, 1);
}

bool UnitPAJ7620U2::write_banked_register(const uint16_t reg, const uint8_t* buf, const size_t len)
{
    return select_bank((reg >> 8) & 1) && writeRegister((uint8_t)(reg & 0xFF), buf, len);
}

bool UnitPAJ7620U2::write_banked_register8(const uint16_t reg, const uint8_t value)
{
    return select_bank((reg >> 8) & 1) && writeRegister8((uint8_t)(reg & 0xFF), value);
}

// GPIO bit-bang wakeup: sends START + slave_addr + W + STOP without using the I2C driver.
// This avoids I2C driver entering INVALID_STATE from the expected NACK during wakeup.
// Uses gpio_set_level/gpio_set_direction to avoid disrupting I2C peripheral pin ownership.
bool UnitPAJ7620U2::wakeup_gpio(const int16_t sda_pin, const int16_t scl_pin)
{
    if (sda_pin < 0 || scl_pin < 0) {
        M5_LIB_LOGE("Invalid pins for GPIO wakeup: SDA:%d SCL:%d", sda_pin, scl_pin);
        return false;
    }

    uint8_t addr_byte = (address() << 1);  // W bit = 0
    gpio_num_t sda    = (gpio_num_t)sda_pin;
    gpio_num_t scl    = (gpio_num_t)scl_pin;

    // Temporarily switch pins to GPIO open-drain output
    gpio_set_direction(sda, GPIO_MODE_OUTPUT_OD);
    gpio_set_direction(scl, GPIO_MODE_OUTPUT_OD);

    // Idle state: both HIGH
    gpio_set_level(sda, 1);
    gpio_set_level(scl, 1);
    delayMicroseconds(10);

    // START condition: SDA goes LOW while SCL is HIGH
    gpio_set_level(sda, 0);
    delayMicroseconds(10);
    gpio_set_level(scl, 0);
    delayMicroseconds(10);

    // Send address byte (MSB first)
    for (int i = 7; i >= 0; --i) {
        gpio_set_level(sda, (addr_byte >> i) & 1);
        delayMicroseconds(5);
        gpio_set_level(scl, 1);
        delayMicroseconds(10);
        gpio_set_level(scl, 0);
        delayMicroseconds(5);
    }

    // ACK/NACK clock pulse (release SDA, NACK expected)
    gpio_set_level(sda, 1);
    delayMicroseconds(5);
    gpio_set_level(scl, 1);
    delayMicroseconds(10);
    gpio_set_level(scl, 0);
    delayMicroseconds(5);

    // STOP condition: SDA goes HIGH while SCL is HIGH
    gpio_set_level(sda, 0);
    delayMicroseconds(5);
    gpio_set_level(scl, 1);
    delayMicroseconds(10);
    gpio_set_level(sda, 1);
    delayMicroseconds(10);

    M5_LIB_LOGI("GPIO wakeup sent on SDA:%d SCL:%d addr:0x%02X", sda_pin, scl_pin, address());
    return true;
}

bool UnitPAJ7620U2::wakeup()
{
    m5::utility::delay(2);  // Wait 700us for PAJ7620U2 to stabilize

    auto ai2c = asAdapter<AdapterI2C>(Adapter::Type::I2C);

    // Use 100kHz for the wakeup sequence — NACK recovery is more reliable at lower clock speeds.
    if (ai2c) {
        ai2c->setClock(100000);
    }

    // If the sensor is still in Operation state (e.g. after ESP32 reset without power cycle), the wakeup sequence will
    // fail. Try to force the sensor into Suspend state first, then wake it up normally. Retry multiple times because
    // the sensor's I2C state may need time to recover after an unclean reset.
    constexpr int max_retries{10};
    for (int attempt = 0; attempt < max_retries; ++attempt) {
        // 1st select_bank: wakeup trigger (NACK expected if sensor is sleeping)
        bool sb1 = select_bank(0, true);
        // Wait for sensor to finish wakeup (datasheet: min 700us)
        m5::utility::delay(2);
        // 2nd select_bank: should ACK if sensor is now awake
        bool sb2 = select_bank(0, true);
        bool wu  = was_wakeup();
        M5_LIB_LOGW("attempt %d: sb1=%d sb2=%d wu=%d", attempt, sb1, sb2, wu);
        if (wu) {
            M5_LIB_LOGI("Wakeup OK at attempt %d", attempt);
            if (ai2c) {
                ai2c->setClock(component_config().clock);
            }
            return true;
        }

        // Force into Suspend regardless of current state
        write_banked_register8(R_TG_ENH, 0x00);                 // Disable PAJ7620U2
        write_banked_register8(SW_SUSPEND_ENL, enter_suspend);  // Enter Suspend
        m5::utility::delay(10);

        M5_LIB_LOGI("Wakeup attempt %d/%d failed", attempt, max_retries);
        m5::utility::delay(100);
    }

    M5_LIB_LOGE("Failed to wait wakeup (I2C)");
    return false;
}

bool UnitPAJ7620U2::wakeup_with_gpio()
{
    m5::utility::delay(2);  // Wait 700us for PAJ7620U2 to stabilize

    auto ai2c       = asAdapter<AdapterI2C>(Adapter::Type::I2C);
    int16_t sda_pin = ai2c ? ai2c->sda() : -1;
    int16_t scl_pin = ai2c ? ai2c->scl() : -1;

    if (sda_pin < 0 || scl_pin < 0) {
        M5_LIB_LOGE("Cannot get SDA/SCL pins for GPIO wakeup");
        return false;
    }

    // Use 100kHz for post-wakeup I2C verification
    if (ai2c) {
        ai2c->setClock(100000);
    }

    // Get Wire pointer (only available for WireImpl; nullptr for BusImpl/I2CClassImpl)
    TwoWire* wire = ai2c ? ai2c->impl()->getWire() : nullptr;

    constexpr int max_retries{10};
    for (int attempt = 0; attempt < max_retries; ++attempt) {
        // Release I2C driver before GPIO bit-bang (GPIO changes pin ownership)
        if (wire) {
            wire->end();
        }

        // GPIO bit-bang wakeup (NACK does not corrupt I2C driver state)
        wakeup_gpio(sda_pin, scl_pin);
        m5::utility::delay(2);  // Wait for sensor to wake up

        // Re-initialize I2C driver (required: GPIO bit-bang invalidates pin ownership)
        if (wire) {
            wire->begin(sda_pin, scl_pin, 100000);
        }

        bool wu = was_wakeup();
        M5_LIB_LOGW("GPIO attempt %d: wu=%d", attempt, wu);
        if (wu) {
            M5_LIB_LOGI("GPIO wakeup OK at attempt %d", attempt);
            if (ai2c) {
                ai2c->setClock(component_config().clock);
            }
            return true;
        }

        // Force into Suspend regardless of current state
        write_banked_register8(R_TG_ENH, 0x00);
        write_banked_register8(SW_SUSPEND_ENL, enter_suspend);
        m5::utility::delay(10);

        M5_LIB_LOGI("GPIO wakeup attempt %d/%d failed", attempt, max_retries);
        m5::utility::delay(100);
    }

    M5_LIB_LOGE("Failed to wait wakeup (GPIO)");
    return false;
}

bool UnitPAJ7620U2::was_wakeup()
{
    uint8_t v{};
    return read_banked_register8(PART_ID_LOW, v) && (v == wakeup_value);
}

bool UnitPAJ7620U2::read_chip_id(uint16_t& id)
{
    uint8_t buf[2]{};
    if (!read_banked_register(PART_ID_LOW, buf, 2)) {
        return false;
    }
    id = static_cast<uint16_t>(buf[1]) << 8 | buf[0];
    return true;
}

bool UnitPAJ7620U2::read_version(uint8_t& version)
{
    return read_banked_register8(VERSION_ID, version);
}

bool UnitPAJ7620U2::start_periodic_measurement(const paj7620u2::Mode mode, const paj7620u2::Frequency freq,
                                               const uint32_t intervalMs)
{
    if (inPeriodic()) {
        return false;
    }
    return writeFrequency(freq) && writeMode(mode) && start_periodic_measurement(intervalMs);
}

bool UnitPAJ7620U2::start_periodic_measurement(const uint32_t intervalMs)
{
    if (inPeriodic()) {
        return false;
    }
    _interval = intervalMs;
    _latest   = 0;
    _periodic = true;
    return true;
}

bool UnitPAJ7620U2::stop_periodic_measurement()
{
    _periodic = false;
    return true;
}

}  // namespace unit
}  // namespace m5
