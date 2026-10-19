/**
 * ,---------,       ____  _ __
 * |  ,-^-,  |      / __ )(_) /_______________ _____  ___
 * | (  O  ) |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * | / ,--´  |    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *    +------`   /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * Crazyflie control firmware
 *
 * Copyright (C) 2026 Bitcraze AB
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, in version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Unit tests for periodicDeadline.h
 */

#include "periodicDeadline.h"

#include "unity.h"

void setUp(void) {}

void tearDown(void) {}

void testDeadlineIsDueAtExactTick(void) {
  TEST_ASSERT_TRUE(periodicDeadlineIsDue(100, 100));
}

void testDeadlineIsNotDueBeforeTick(void) {
  TEST_ASSERT_FALSE(periodicDeadlineIsDue(99, 100));
}

void testAdvancePreservesScheduleAfterLateWake(void) {
  TEST_ASSERT_EQUAL_UINT32(20, periodicDeadlineAdvance(10, 12, 10));
  TEST_ASSERT_EQUAL_UINT32(30, periodicDeadlineAdvance(20, 21, 10));
}

void testAdvanceSkipsMissedPeriodsWithoutBursting(void) {
  TEST_ASSERT_EQUAL_UINT32(60, periodicDeadlineAdvance(30, 56, 10));
}

void testDeadlineAdvanceWrapsUint32Clock(void) {
  const uint32_t deadline = UINT32_MAX - 4;
  TEST_ASSERT_EQUAL_UINT32(
      5, periodicDeadlineAdvance(deadline, UINT32_MAX - 1, 10));
}

void testAdvanceLeavesFutureDeadlineUnchanged(void) {
  TEST_ASSERT_EQUAL_UINT32(100, periodicDeadlineAdvance(100, 99, 10));
}

void testAdvanceLeavesDeadlineUnchangedForZeroInterval(void) {
  TEST_ASSERT_EQUAL_UINT32(100, periodicDeadlineAdvance(100, 100, 0));
}
