#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <float.h>
#include <math.h>

#include "physicalConstants.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "kalman_core.h"

#define MAX_INITIAL_POSITION_METERS 100.0f

typedef struct {
  float x;
  float y;
  float z;
  float yaw;
} estimatorKalmanPose_t;

typedef void (*estimatorKalmanPoseApplyFn)(void* context, const estimatorKalmanPose_t* pose);

static inline void estimatorKalmanPoseApplyToParams(kalmanCoreParams_t* params, const estimatorKalmanPose_t* pose) {
  params->initialX = pose->x;
  params->initialY = pose->y;
  params->initialZ = pose->z;
  params->initialYaw = pose->yaw;
}

static inline bool estimatorKalmanPoseMake(estimatorKalmanPose_t* pose, float x, float y, float z, float yaw) {
  if (pose == NULL || !isfinite(x) || !isfinite(y) || !isfinite(z) || !isfinite(yaw) ||
      fabsf(x) > MAX_INITIAL_POSITION_METERS ||
      fabsf(y) > MAX_INITIAL_POSITION_METERS ||
      fabsf(z) > MAX_INITIAL_POSITION_METERS) {
    return false;
  }

  pose->x = x;
  pose->y = y;
  pose->z = z;
  pose->yaw = remainderf(yaw, 2.0f * M_PI_F);
  if (pose->yaw >= M_PI_F - 4.0f * FLT_EPSILON * M_PI_F) {
    pose->yaw -= 2.0f * M_PI_F;
  }
  return true;
}

static inline bool estimatorKalmanPoseQueueSubmit(QueueHandle_t queue, float x, float y, float z, float yaw) {
  estimatorKalmanPose_t pose;
  return estimatorKalmanPoseMake(&pose, x, y, z, yaw) && xQueueSend(queue, &pose, 0) == pdPASS;
}

static inline size_t estimatorKalmanPoseQueueDrain(
    QueueHandle_t queue, estimatorKalmanPoseApplyFn apply, void* context) {
  size_t applied = 0;
  estimatorKalmanPose_t pose;
  while (apply != NULL && xQueueReceive(queue, &pose, 0) == pdPASS) {
    apply(context, &pose);
    applied++;
  }
  return applied;
}
