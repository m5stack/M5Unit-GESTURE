/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file M5UnitUnifiedGESTURE.hpp
  @brief Main header of M5UnitGESTURE

  @mainpage M5UnitGESTURE
  Library for UnitGESTURE using M5UnitUnified.
*/
#ifndef M5_UNIT_UNIFIED_GESTURE_HPP
#define M5_UNIT_UNIFIED_GESTURE_HPP

#include "unit/unit_PAJ7620U2.hpp"

/*!
  @namespace m5
  @brief Top level namespace of M5stack
 */
namespace m5 {

/*!
  @namespace unit
  @brief Unit-related namespace
 */
namespace unit {
using UnitGESTURE [[deprecated("Please use UnitGesture")]] = m5::unit::UnitPAJ7620U2;
using UnitGesture = m5::unit::UnitPAJ7620U2;
}  // namespace unit
}  // namespace m5
#endif
