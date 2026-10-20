// File under test kalman_core.c
#include "kalman_core.h"

#include "physicalConstants.h"
#include "unity.h"

// @BUILD_LIB ARM_DSP_MATH

static kalmanCoreData_t core;
static kalmanCoreParams_t params;

void setUp(void) {
  memset(&core, 0, sizeof(core));
  kalmanCoreDefaultParams(&params);
  kalmanCoreInit(&core, &params, 10);
}

void tearDown(void) {
}

void testSkippedStartupPredictionDoesNotIntegrateTheMissingImuInterval(void) {
  // The estimator can wait for gyro calibration before receiving its first complete IMU batch.
  kalmanCoreSkipPrediction(&core, 2070);

  Axis3f acceleration = {.x = 0.0f, .y = 0.0f, .z = GRAVITY_MAGNITUDE};
  Axis3f gyro = {.x = 0.0f, .y = 0.0f, .z = 0.0f};
  kalmanCorePredict(&core, &acceleration, &gyro, 2080, false);

  TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, core.S[KC_STATE_PZ]);
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, core.S[KC_STATE_Z]);
  TEST_ASSERT_EQUAL_UINT32(2080, core.lastPredictionMs);
}

void testUnskippedStartupPredictionWouldIntegrateTheWholeWaitAsFreeFall(void) {
  Axis3f zeroAcceleration = {.x = 0.0f, .y = 0.0f, .z = 0.0f};
  Axis3f zeroGyro = {.x = 0.0f, .y = 0.0f, .z = 0.0f};
  kalmanCorePredict(&core, &zeroAcceleration, &zeroGyro, 2070, false);

  TEST_ASSERT_TRUE(core.S[KC_STATE_PZ] < -10.0f);
}
