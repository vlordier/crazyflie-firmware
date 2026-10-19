/**
 * ,---------,       ____  _ __
 * |  ,-^-,  |      / __ )(_) /_______________ _____  ___
 * | (  O  ) |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * | / ,--´  |    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *    +------`   /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * Crazyflie control firmware
 *
 * Copyright (C) 20 Bitcraze AB
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
 *
 * test_rateSupervisor.h - unit tests for rate supervisor
 */

// File under test
#include "rateSupervisor.h"

#include "unity.h"


static rateSupervisor_t context;

static uint32_t startTime = 12345;
static uint32_t evaluationIntervall = 1000;
static uint32_t minCount = 2;
static uint32_t maxCount = 4;

void setUp(void) {
    rateSupervisorInit(&context, startTime, evaluationIntervall, minCount, maxCount, 0);
}

void tearDown(void) {
  // Empty
}

void testThatValidationPassesBeforeEvaluation() {
    // Fixture
    // Test
    bool actual = rateSupervisorValidate(&context, startTime + 100);

    // Assert
    TEST_ASSERT_TRUE(actual);
}

void testThatValidationPassesWhenCountIsWithinInterval() {
    // Fixture
    rateSupervisorValidate(&context, startTime + 400);
    rateSupervisorValidate(&context, startTime + 800);

    // Test
    bool actual = rateSupervisorValidate(&context, startTime + 1200);

    // Assert
    TEST_ASSERT_TRUE(actual);
}

void testThatValidationFailesWhenCountIsTooLow() {
    // Fixture

    // Test
    bool actual = rateSupervisorValidate(&context, startTime + 1200);

    // Assert
    TEST_ASSERT_FALSE(actual);
}

void testThatValidationPassesWhenCountIsTooHigh() {
    // Fixture
    rateSupervisorValidate(&context, startTime + 400);
    rateSupervisorValidate(&context, startTime + 500);
    rateSupervisorValidate(&context, startTime + 600);
    rateSupervisorValidate(&context, startTime + 700);

    // Test
    bool actual = rateSupervisorValidate(&context, startTime + 1200);

    // Assert
    TEST_ASSERT_FALSE(actual);
}

void testThatCounterIsResetAtEvaluation() {
    // Fixture
    rateSupervisorValidate(&context, startTime + 400);
    rateSupervisorValidate(&context, startTime + 700);
    rateSupervisorValidate(&context, startTime + 1200);
    // An evaluation should be triggered by the last call and counter reset

    // Test
    bool actual = rateSupervisorValidate(&context, startTime + 1200 + 1100);

    // Assert
    // The test call should trigger a second evaluation, and since the counter was
    // reset, the validation should return false
    TEST_ASSERT_FALSE(actual);
}

void testThatLatestCountIsStored() {
    // Fixture
    rateSupervisorValidate(&context, startTime + 1200);
    uint32_t expected = 1;

    // Test
    uint32_t actual = rateSupervisorLatestCount(&context);

    // Assert
    TEST_ASSERT_EQUAL_UINT32(expected, actual);
}

void testThatValidationPassesWithSkip() {
    // Fixture
    uint32_t skip = 1;
    rateSupervisorInit(&context, startTime, evaluationIntervall, minCount, maxCount, skip);

    // Test
    bool actual = rateSupervisorValidate(&context, startTime + 1200);

    // Assert
    TEST_ASSERT_TRUE(actual);
}

void testThat500HzRateIsAcceptedBy500HzBounds() {
    rateSupervisorInit(&context, startTime, evaluationIntervall, 498, 501, 0);

    // The first stabilizer sample may already be queued when rate checking starts.
    rateSupervisorValidate(&context, startTime);
    bool actual = true;
    for (uint32_t elapsedMs = 2; elapsedMs <= 1000; elapsedMs += 2) {
        actual = rateSupervisorValidate(&context, startTime + elapsedMs);
    }

    TEST_ASSERT_TRUE(actual);
    TEST_ASSERT_EQUAL_UINT32(501, rateSupervisorLatestCount(&context));

    for (uint32_t elapsedMs = 1002; elapsedMs <= 2000; elapsedMs += 2) {
        actual = rateSupervisorValidate(&context, startTime + elapsedMs);
    }

    TEST_ASSERT_TRUE(actual);
    TEST_ASSERT_EQUAL_UINT32(500, rateSupervisorLatestCount(&context));
}

void testThat1000HzRateIsAcceptedBy1000HzBounds() {
    rateSupervisorInit(&context, startTime, evaluationIntervall, 997, 1003, 0);

    rateSupervisorValidate(&context, startTime);
    bool actual = true;
    for (uint32_t elapsedMs = 1; elapsedMs <= 1000; elapsedMs++) {
        actual = rateSupervisorValidate(&context, startTime + elapsedMs);
    }

    TEST_ASSERT_TRUE(actual);
    TEST_ASSERT_EQUAL_UINT32(1001, rateSupervisorLatestCount(&context));
}

void testThatUnderspeedIsRejectedBy500HzBounds() {
    rateSupervisorInit(&context, startTime, evaluationIntervall, 498, 501, 0);

    bool actual = true;
    for (uint32_t sample = 1; sample <= 460; sample++) {
        const uint32_t elapsedMs = sample * 1000 / 460;
        actual = rateSupervisorValidate(&context, startTime + elapsedMs);
    }

    TEST_ASSERT_FALSE(actual);
    TEST_ASSERT_EQUAL_UINT32(460, rateSupervisorLatestCount(&context));
}

void testThatValidationFailesAfterSkip() {
    // Fixture
    uint32_t skip = 1;
    rateSupervisorInit(&context, startTime, evaluationIntervall, minCount, maxCount, skip);
    rateSupervisorValidate(&context, startTime + 1200);

    // Test
    bool actual = rateSupervisorValidate(&context, startTime + 1200 + 1100);

    // Assert
    TEST_ASSERT_FALSE(actual);
}
