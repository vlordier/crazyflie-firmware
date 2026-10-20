# Original author: https://github.com/wuwushrek
# Modified by: Christian Llanes (https://github.com/llanesc)

cmake_minimum_required(VERSION 3.11.0 FATAL_ERROR)
cmake_policy(SET CMP0042 NEW)
cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0054 NEW)

if (NOT CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr" CACHE STRING "install prefix" FORCE)
endif()

project(cf2_sitl VERSION 1.0.0)
include(GNUInstallDirs)

set(CRAZYSIM_RATE_MAIN_LOOP "1000" CACHE STRING "CrazySim stabilizer loop frequency in Hz (500 or 1000)")
if(NOT CRAZYSIM_RATE_MAIN_LOOP STREQUAL "500" AND NOT CRAZYSIM_RATE_MAIN_LOOP STREQUAL "1000")
  message(FATAL_ERROR "CRAZYSIM_RATE_MAIN_LOOP must be 500 or 1000")
endif()
option(CRAZYSIM_DIAGNOSTICS "Enable lightweight SITL readiness and command markers" OFF)
option(CRAZYSIM_VERBOSE_DIAGNOSTICS "Print sampled SITL IMU and stabilizer state in real time" OFF)
option(CRAZYSIM_TRACE_RING "Capture bounded per-loop SITL state and motor trace" OFF)
option(CRAZYSIM_IMU_PROVENANCE "Capture sequence, source time, and raw IMU provenance in the SITL trace" OFF)
option(CRAZYSIM_TRACE_DIAGNOSTICS "Capture ring overflow and skipped-record counters" OFF)
option(CRAZYSIM_KALMAN_TRACE "Capture Kalman prediction counts in the SITL trace ring" OFF)
option(CRAZYSIM_NANO_APP "Build Nano-C-SLAM application sources into cf2 SITL" OFF)
set(CRAZYSIM_NANO_APP_DIR "" CACHE PATH "Path to cslam-stm32-app")
set(CRAZYSIM_NANO_UWB_DIR "" CACHE PATH "Path to uwb-software-library")
set(CRAZYSIM_NANO_NODE_ID "0" CACHE STRING "Nano SITL node ID (0-5)")
set(CRAZYSIM_TRACE_RING_CAPACITY "32768" CACHE STRING "Maximum SITL diagnostic trace records")
if(CRAZYSIM_TRACE_DIAGNOSTICS AND NOT CRAZYSIM_TRACE_RING)
  message(FATAL_ERROR "CRAZYSIM_TRACE_DIAGNOSTICS requires CRAZYSIM_TRACE_RING=ON")
endif()
if(CRAZYSIM_IMU_PROVENANCE AND NOT CRAZYSIM_TRACE_RING)
  message(FATAL_ERROR "CRAZYSIM_IMU_PROVENANCE requires CRAZYSIM_TRACE_RING=ON")
endif()
if(CRAZYSIM_KALMAN_TRACE AND NOT CRAZYSIM_TRACE_RING)
  message(FATAL_ERROR "CRAZYSIM_KALMAN_TRACE requires CRAZYSIM_TRACE_RING=ON")
endif()

#######################
## Find Dependencies ##
#######################

# Add search directory for CMake on OS X
list(APPEND CMAKE_MODULE_PATH /usr/local/share/cmake/Modules)
# list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

## System dependencies are found with CMake's conventions
find_package(PkgConfig REQUIRED)

# find thread package
find_package (Threads REQUIRED)


set (CF2_SITL_ROOT_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/../
)

set (CF2_SITL_SRCS_DIR
  ${CF2_SITL_ROOT_DIR}/src
)

set(CF2_SITL_BINARY_DIR 
  ${CMAKE_SOURCE_DIR}/build
)

set(FREERTOS_SRC_DIR 
  ${CF2_SITL_ROOT_DIR}/vendor/FreeRTOS
)

# Code variables
set(ESTIMATOR
  Kalman #Kalman, Complementary, Ukf
)
set(CONTROLLER
  PID #PID, Mellinger
)

#Generate version.c dependencies
add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../version.c
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../
  COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/../tools/make/versionTemplate.py ${CMAKE_CURRENT_SOURCE_DIR}/../src/utils/src/version.vtpl ${CMAKE_CURRENT_SOURCE_DIR}/../version.c
  )
add_custom_target(version_cf ALL DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../version.c)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -Wno-unused-variable -std=gnu11 -g3 -Wall -Wno-maybe-uninitialized -Wno-error=maybe-uninitialized -Wmissing-braces -fno-strict-aliasing -ffunction-sections -fdata-sections -Wdouble-promotion -Wno-deprecated-declarations")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DESTIMATOR=StateEstimatorType${ESTIMATOR} -DCONTROLLER=ControllerType${CONTROLLER} -DCONFIG_SENSORS_SITL -DSENSORS_FORCE=SensorImplementation_sitl")
if(CRAZYSIM_DIAGNOSTICS)
  add_compile_definitions(NCS_SITL_DIAGNOSTICS)
endif()
if(CRAZYSIM_VERBOSE_DIAGNOSTICS)
  add_compile_definitions(NCS_SITL_VERBOSE_DIAGNOSTICS)
endif()
if(CRAZYSIM_TRACE_RING)
  if(NOT CRAZYSIM_TRACE_RING_CAPACITY MATCHES "^[1-9][0-9]*$")
    message(FATAL_ERROR "CRAZYSIM_TRACE_RING_CAPACITY must be a positive integer")
  endif()
  if(NOT DEFINED CRAZYSIM_TRACE_RING_STRIDE)
    set(CRAZYSIM_TRACE_RING_STRIDE 1)
  endif()
  if(NOT CRAZYSIM_TRACE_RING_STRIDE MATCHES "^[1-9][0-9]*$")
    message(FATAL_ERROR "CRAZYSIM_TRACE_RING_STRIDE must be a positive integer")
  endif()
  add_compile_definitions(
    NCS_SITL_TRACE_RING
    NCS_SITL_TRACE_RING_CAPACITY=${CRAZYSIM_TRACE_RING_CAPACITY}
    NCS_SITL_TRACE_STRIDE=${CRAZYSIM_TRACE_RING_STRIDE}
  )
endif()
if(CRAZYSIM_KALMAN_TRACE)
  add_compile_definitions(NCS_SITL_KALMAN_TRACE)
endif()

#set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} --specs=nano.specs -T ${CF2_SITL_SRCS_DIR}/../tools/make/F405/linker/FLASH.ld")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wno-deprecated-declarations")
set(CMAKE_BUILD_TYPE Release)

set(${PROJECT_NAME}_INCLUDE_DIRS
    ${PROJECT_SOURCE_DIR}/include
    CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)

set(FREERTOS_SRCS
   ${FREERTOS_SRC_DIR}/event_groups.c
   ${FREERTOS_SRC_DIR}/list.c
   ${FREERTOS_SRC_DIR}/queue.c
   ${FREERTOS_SRC_DIR}/tasks.c
   ${FREERTOS_SRC_DIR}/timers.c
   ${FREERTOS_SRC_DIR}/croutine.c
   ${FREERTOS_SRC_DIR}/portable/ThirdParty/GCC/Posix/port.c
   ${FREERTOS_SRC_DIR}/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c
   ${FREERTOS_SRC_DIR}/portable/MemMang/heap_3.c
)

set (CF2_SITL_INCLUDE_DIRS
  ${FREERTOS_SRC_DIR}/include
  ${FREERTOS_SRC_DIR}/portable/ThirdParty/GCC/Posix
  ${FREERTOS_SRC_DIR}/portable/ThirdParty/GCC/Posix/utils
  ${CF2_SITL_SRCS_DIR}/config
  ${CF2_SITL_SRCS_DIR}/drivers/interface
  #${CF2_SITL_SRCS_DIR}/deck/interface
  #${CF2_SITL_SRCS_DIR}/deck/drivers/interface
  ${CF2_SITL_SRCS_DIR}/hal/interface
  ${CF2_SITL_SRCS_DIR}/platform
  ${CF2_SITL_SRCS_DIR}/utils/interface
  ${CF2_SITL_SRCS_DIR}/utils/interface/lighthouse
  ${CF2_SITL_SRCS_DIR}/utils/interface/kve
  ${CF2_SITL_SRCS_DIR}/platform/interface
  ${CF2_SITL_SRCS_DIR}/modules/interface
  ${CF2_SITL_SRCS_DIR}/modules/interface/controller
  ${CF2_SITL_SRCS_DIR}/modules/interface/estimator
  ${CF2_SITL_SRCS_DIR}/modules/interface/kalman_core
  ${CF2_SITL_SRCS_DIR}/modules/interface/outlierfilter

  #Verification
  ${INCL_VERIF}
  #${CMAKE_CURRENT_SOURCE_DIR}/include
)

include_directories(${CF2_SITL_INCLUDE_DIRS})

set(SRCS
  # main
  ${CF2_SITL_SRCS_DIR}/init/main_sitl.c

  #Hal
  ${CF2_SITL_SRCS_DIR}/hal/src/usec_time_sitl.c
  ${CF2_SITL_SRCS_DIR}/hal/src/buzzer.c
  ${CF2_SITL_SRCS_DIR}/hal/src/storage.c
  ${CF2_SITL_SRCS_DIR}/hal/src/pm_sitl.c
  ${CF2_SITL_SRCS_DIR}/hal/src/radiolink_sitl.c

  # ${CF2_SITL_SRCS_DIR}/deck/core/deck_drivers.c
  # ${CF2_SITL_SRCS_DIR}/deck/core/deck_info.c
  # ${CF2_SITL_SRCS_DIR}/deck/core/deck_memory.c
  # ${CF2_SITL_SRCS_DIR}/deck/core/deck_test.c
  # ${CF2_SITL_SRCS_DIR}/deck/core/deck.c

  # SITL deck stub (exposes deck parameters for cfclient)
  ${CF2_SITL_SRCS_DIR}/deck/drivers/src/deck_sitl.c

  #LED SITL stubs
  ${CF2_SITL_SRCS_DIR}/drivers/src/led_sitl.c
  ${CF2_SITL_SRCS_DIR}/hal/src/ledseq.c
  ${CF2_SITL_SRCS_DIR}/deck/drivers/src/colorled_sitl.c

  #Communication
  ${CF2_SITL_SRCS_DIR}/hal/src/socketlink.c
  ${CF2_SITL_SRCS_DIR}/modules/src/crtp.c

  #sensors
  ${CF2_SITL_SRCS_DIR}/hal/src/sensors_sitl.c
  ${CF2_SITL_SRCS_DIR}/hal/src/sensors.c

  #Range
  ${CF2_SITL_SRCS_DIR}/modules/src/range.c

  #Modules
  ${CF2_SITL_SRCS_DIR}/modules/src/system.c
  ${CF2_SITL_SRCS_DIR}/modules/src/comm.c
  ${CF2_SITL_SRCS_DIR}/modules/src/console.c
  ${CF2_SITL_SRCS_DIR}/utils/src/pid.c
  ${CF2_SITL_SRCS_DIR}/modules/src/crtpservice.c
  ${CF2_SITL_SRCS_DIR}/modules/src/param_task.c
  ${CF2_SITL_SRCS_DIR}/modules/src/param_logic.c
  ${CF2_SITL_SRCS_DIR}/modules/src/log.c
  ${CF2_SITL_SRCS_DIR}/modules/src/worker.c
  ${CF2_SITL_SRCS_DIR}/modules/src/queuemonitor.c
  ${CF2_SITL_SRCS_DIR}/modules/src/msp.c
  ${CF2_SITL_SRCS_DIR}/modules/src/platformservice.c
  ${CF2_SITL_SRCS_DIR}/modules/src/sysload.c
  ${CF2_SITL_SRCS_DIR}/modules/src/crtp_mem.c
  ${CF2_SITL_SRCS_DIR}/modules/src/mem.c
  ${CF2_SITL_SRCS_DIR}/modules/src/static_mem.c
  ${CF2_SITL_SRCS_DIR}/modules/src/pptraj_compressed.c
  ${CF2_SITL_SRCS_DIR}/modules/src/pptraj.c
  ${CF2_SITL_SRCS_DIR}/modules/src/supervisor.c
  ${CF2_SITL_SRCS_DIR}/modules/src/supervisor_state_machine.c
  ${CF2_SITL_SRCS_DIR}/modules/src/app_channel.c
  ${CF2_SITL_SRCS_DIR}/modules/src/peer_localization.c
  ${CF2_SITL_SRCS_DIR}/modules/src/axis3fSubSampler.c
  #stabilizer modules
  ${CF2_SITL_SRCS_DIR}/modules/src/commander.c
  ${CF2_SITL_SRCS_DIR}/modules/src/crtp_commander.c
  ${CF2_SITL_SRCS_DIR}/modules/src/crtp_commander_rpyt.c
  ${CF2_SITL_SRCS_DIR}/modules/src/crtp_commander_generic.c
  ${CF2_SITL_SRCS_DIR}/modules/src/crtp_localization_service.c
  ${CF2_SITL_SRCS_DIR}/modules/src/sensfusion6.c
  ${CF2_SITL_SRCS_DIR}/modules/src/stabilizer.c
  ${CF2_SITL_SRCS_DIR}/modules/src/eventtrigger.c
  ${CF2_SITL_SRCS_DIR}/modules/src/estimator/position_estimator_altitude.c
  ${CF2_SITL_SRCS_DIR}/modules/src/estimator/estimator.c
  ${CF2_SITL_SRCS_DIR}/modules/src/estimator/estimator_complementary.c
  ${CF2_SITL_SRCS_DIR}/modules/src/estimator/estimator_kalman.c
  ${CF2_SITL_SRCS_DIR}/modules/src/kalman_core/kalman_core.c
  ${CF2_SITL_SRCS_DIR}/modules/src/kalman_core/mm_absolute_height.c
  # ${CF2_SITL_SRCS_DIR}/modules/src/kalman_core/mm_distance_robust.c
  ${CF2_SITL_SRCS_DIR}/modules/src/kalman_core/mm_distance.c
  ${CF2_SITL_SRCS_DIR}/modules/src/kalman_core/mm_flow.c
  ${CF2_SITL_SRCS_DIR}/modules/src/kalman_core/mm_pose.c
  ${CF2_SITL_SRCS_DIR}/modules/src/kalman_core/mm_position.c
  ${CF2_SITL_SRCS_DIR}/modules/src/kalman_core/mm_sweep_angles.c
  # ${CF2_SITL_SRCS_DIR}/modules/src/kalman_core/mm_tdoa_robust.c
  ${CF2_SITL_SRCS_DIR}/modules/src/kalman_core/mm_tdoa.c
  ${CF2_SITL_SRCS_DIR}/modules/src/kalman_core/mm_tof.c
  ${CF2_SITL_SRCS_DIR}/modules/src/kalman_core/mm_yaw_error.c
  ${CF2_SITL_SRCS_DIR}/modules/src/kalman_supervisor.c

  ${CF2_SITL_SRCS_DIR}/modules/src/estimator/estimator_ukf.c
  ${CF2_SITL_SRCS_DIR}/modules/src/outlierfilter/outlierFilterTdoa.c
  ${CF2_SITL_SRCS_DIR}/modules/src/outlierfilter/outlierFilterTdoaSteps.c
  ${CF2_SITL_SRCS_DIR}/modules/src/outlierfilter/outlierFilterLighthouse.c

  #Controller and power distribution
  ${CF2_SITL_SRCS_DIR}/modules/src/controller/attitude_pid_controller.c
  ${CF2_SITL_SRCS_DIR}/modules/src/controller/position_controller_pid.c
  ${CF2_SITL_SRCS_DIR}/modules/src/controller/controller.c
  ${CF2_SITL_SRCS_DIR}/modules/src/controller/controller_pid.c
  ${CF2_SITL_SRCS_DIR}/modules/src/controller/controller_mellinger.c
  ${CF2_SITL_SRCS_DIR}/modules/src/controller/controller_indi.c
  ${CF2_SITL_SRCS_DIR}/modules/src/controller/position_controller_indi.c
  ${CF2_SITL_SRCS_DIR}/modules/src/controller/controller_brescianini.c
  ${CF2_SITL_SRCS_DIR}/modules/src/power_distribution_sitl.c

  #High level commander
  ${CF2_SITL_SRCS_DIR}/modules/src/crtp_commander_high_level.c
  ${CF2_SITL_SRCS_DIR}/modules/src/planner.c
  ${CF2_SITL_SRCS_DIR}/modules/src/pptraj.c

  #Platform
  ${CF2_SITL_SRCS_DIR}/platform/src/platform.c
  ${CF2_SITL_SRCS_DIR}/platform/src/platform_sitl.c

  #Utilities
  ${CF2_SITL_SRCS_DIR}/utils/src/filter.c
  ${CF2_SITL_SRCS_DIR}/utils/src/eprintf.c
  ${CF2_SITL_SRCS_DIR}/utils/src/crc32.c
  ${CF2_SITL_SRCS_DIR}/utils/src/num.c
  ${CF2_SITL_SRCS_DIR}/utils/src/debug.c
  ${CF2_SITL_SRCS_DIR}/utils/src/configblocksitl.c
  ${CF2_SITL_SRCS_DIR}/utils/src/cfassert.c
  ${CF2_SITL_SRCS_DIR}/utils/src/rateSupervisor.c
  ${CF2_SITL_SRCS_DIR}/utils/src/statsCnt.c
  ${CF2_SITL_SRCS_DIR}/utils/src/kve/kve.c
  ${CF2_SITL_SRCS_DIR}/utils/src/kve/kve_storage.c

  ${CF2_SITL_SRCS_DIR}/../version.c
  ${FREERTOS_SRCS}

  #Verification include
  ${SRC_VERIF}
)

if(CRAZYSIM_NANO_APP)
  if(NOT IS_DIRECTORY "${CRAZYSIM_NANO_APP_DIR}")
    message(FATAL_ERROR "Set CRAZYSIM_NANO_APP_DIR to the cslam-stm32-app checkout")
  endif()
  if(NOT EXISTS "${CRAZYSIM_NANO_APP_DIR}/app_main.c")
    message(FATAL_ERROR "CRAZYSIM_NANO_APP_DIR must contain app_main.c")
  endif()
  if(NOT IS_DIRECTORY "${CRAZYSIM_NANO_UWB_DIR}")
    message(FATAL_ERROR "Set CRAZYSIM_NANO_UWB_DIR to uwb-software-library")
  endif()
  if(NOT CRAZYSIM_NANO_NODE_ID MATCHES "^[0-5]$")
    message(FATAL_ERROR "CRAZYSIM_NANO_NODE_ID must be an integer from 0 through 5")
  endif()

  include("${CRAZYSIM_NANO_APP_DIR}/sitl/nano_sources.cmake")
  set(NCS_NANO_APP_C_SOURCES)
  foreach(source IN LISTS NCS_NANO_APP_SOURCES)
    if(NOT EXISTS "${CRAZYSIM_NANO_APP_DIR}/${source}")
      message(FATAL_ERROR "Nano SITL source manifest entry is missing: ${source}")
    endif()
    list(APPEND NCS_NANO_APP_C_SOURCES "${CRAZYSIM_NANO_APP_DIR}/${source}")
  endforeach()

  list(APPEND SRCS
    ${CF2_SITL_SRCS_DIR}/modules/src/app_handler.c
    ${CRAZYSIM_NANO_APP_DIR}/app_main.c
    ${CRAZYSIM_NANO_APP_DIR}/sitl/tof_matrix_sitl.c
    ${CRAZYSIM_NANO_APP_DIR}/sitl/uwb_api_sitl.c
    ${CRAZYSIM_NANO_APP_DIR}/sitl/uwb_sitl_protocol.c
    ${CRAZYSIM_NANO_APP_DIR}/sitl/spi_transport_sitl.c
    ${CRAZYSIM_NANO_UWB_DIR}/uwb_api_message_utils.c
    ${NCS_NANO_APP_C_SOURCES}
  )
endif()

# Create an executable
add_executable(cf2 ${SRCS})

if(CRAZYSIM_NANO_APP)
  target_include_directories(cf2 PRIVATE
    ${CRAZYSIM_NANO_APP_DIR}/src
    ${CRAZYSIM_NANO_APP_DIR}/src/exploration
    ${CRAZYSIM_NANO_APP_DIR}/src/spi_comm
    ${CRAZYSIM_NANO_APP_DIR}/src/tof-matrix
    ${CRAZYSIM_NANO_APP_DIR}/src/tof-matrix/drivers
    ${CRAZYSIM_NANO_UWB_DIR}/inc
    ${CRAZYSIM_NANO_UWB_DIR}/decawave_api
    ${CF2_SITL_SRCS_DIR}/deck/interface
  )
  target_compile_definitions(cf2 PRIVATE
    CONFIG_APP_ENABLE
    CONFIG_APP_PRIORITY=1
    CONFIG_APP_STACKSIZE=1000
    NCS_SITL_NANO_APP
    NCS_SITL_NANO_NODE_ID=${CRAZYSIM_NANO_NODE_ID}
  )
endif()

target_link_libraries(cf2 ${CMAKE_THREAD_LIBS_INIT} m)

target_compile_options(cf2 PUBLIC -DCONFIG_DECK_FORCE -DCONFIG_ESTIMATOR_KALMAN_ENABLE -DCONFIG_ESTIMATOR_UKF_ENABLE -DCRAZYFLIE_FW -DCONFIG_PLATFORM_SITL -DARCH_64 -DRATE_MAIN_LOOP=${CRAZYSIM_RATE_MAIN_LOOP})
target_compile_definitions(cf2 PUBLIC CONFIG_PLATFORM_SITL)
if(CRAZYSIM_IMU_PROVENANCE)
  target_compile_definitions(cf2 PUBLIC NCS_SITL_IMU_PROVENANCE)
endif()
if(CRAZYSIM_TRACE_DIAGNOSTICS)
  target_compile_definitions(cf2 PUBLIC NCS_SITL_TRACE_DIAGNOSTICS)
endif()

# Check if linker supports --no-warn-rwx-segments (added in binutils 2.39)
execute_process(
  COMMAND ${CMAKE_LINKER} --help
  OUTPUT_VARIABLE LINKER_HELP_OUTPUT
  ERROR_QUIET
)
string(FIND "${LINKER_HELP_OUTPUT}" "--no-warn-rwx-segments" LINKER_SUPPORTS_NO_WARN_RWX)

if(LINKER_SUPPORTS_NO_WARN_RWX GREATER -1)
  set(NO_WARN_RWX_FLAG ",--no-warn-rwx-segments")
else()
  set(NO_WARN_RWX_FLAG "")
endif()

set_target_properties(
  cf2 PROPERTIES LINK_FLAGS "-Wl,--gc-sections${NO_WARN_RWX_FLAG},-T${CMAKE_CURRENT_SOURCE_DIR}/log_param_linker.ld"
)

include(cmake/sitl_gazebo.cmake)
