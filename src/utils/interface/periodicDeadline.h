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
 * Wrap-safe helpers for periodic deadlines measured in a uint32_t tick clock.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

// The deadline must be less than 2^31 ticks away for this comparison to be
// unambiguous across uint32_t wraparound.
static inline bool periodicDeadlineIsDue(const uint32_t now,
                                         const uint32_t deadline) {
  return (uint32_t)(now - deadline) < 0x80000000U;
}

// Advance from the scheduled deadline, not the actual wake time, while
// skipping missed periods so a delayed task does not run catch-up bursts.
// Returns the original deadline if interval is zero or the deadline is future.
static inline uint32_t periodicDeadlineAdvance(const uint32_t deadline,
                                               const uint32_t now,
                                               const uint32_t interval) {
  if (interval == 0 || !periodicDeadlineIsDue(now, deadline)) {
    return deadline;
  }

  const uint32_t elapsed = now - deadline;
  const uint32_t periods = elapsed / interval + 1;
  return deadline + periods * interval;
}
