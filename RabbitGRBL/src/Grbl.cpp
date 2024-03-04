/*
  Grbl.cpp - Initialization and main loop for Grbl
  Part of Grbl
  Copyright (c) 2014-2016 Sungeun K. Jeon for Gnea Research LLC

    2018 -	Bart Dring This file was modifed for use on the ESP32
                    CPU. Do not use this with Grbl for atMega328P

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Grbl.h"

void grbl_init()
{
    client_init(); // Setup serial baud rate and interrupts
    display_init();

    // show the map name at startup

    settings_init(); // Load Grbl settings from non-volatile storage
    stepper_init();  // Configure stepper pins and interrupt timers
    system_ini();    // Configure pinout pins and pin-change interrupt (Renamed due to conflict with esp32 files)
    backlash_ini();
    init_motors();
    memset(sys_position, 0, sizeof(sys_position)); // Clear machine position.
    machine_init();                                // weak definition in Grbl.cpp does nothing
    // Initialize system state.
#ifdef FORCE_INITIALIZATION_ALARM
    // Force Grbl into an ALARM state upon a power-cycle or hard reset.
    sys.state = State::Alarm;
#else
    sys.state = State::Idle;
#endif
    // Check for power-up and set system alarm if homing is enabled to force homing cycle
    // by setting Grbl's alarm state. Alarm locks out all g-code commands, including the
    // startup scripts, but allows access to settings and internal commands. Only a homing
    // cycle '$H' or kill alarm locks '$X' will disable the alarm.
    // NOTE: The startup script will run after successful completion of the homing cycle, but
    // not after disabling the alarm locks. Prevents motion startup blocks from crashing into
    // things uncontrollably. Very bad.
#ifdef HOMING_INIT_LOCK
    if (homing_enable->get())
    {
        sys.state = State::Alarm;
    }
#endif

    Spindles::Spindle::select();
    fSerialInputBuffer.begin();
}

static void reset_variables()
{
    // Reset system variables.
    State prior_state = sys.state;
    memset(&sys, 0, sizeof(system_t)); // Clear system struct variable.
    sys.state = prior_state;
    sys.f_override = FeedOverride::Default;                    // Set to 100%
    sys.r_override = RapidOverride::Default;                   // Set to 100%
    sys.spindle_speed_ovr = SpindleSpeedOverride::Default;     // Set to 100%
    memset(sys_probe_position, 0, sizeof(sys_probe_position)); // Clear probe position.

    Probe::setSystemProbeState(false);

    sys_rt_exec_state.value = 0;
    sys_rt_exec_accessory_override.value = 0;
    sys_rt_exec_alarm = ExecAlarm::None;
    cycle_stop = false;

    // Reset overrides
    sys_rt_f_override = FeedOverride::Default;
    sys_rt_r_override = RapidOverride::Default;
    sys_rt_s_override = SpindleSpeedOverride::Default;

    // Reset Grbl primary systems.
    client_reset_read_buffer();
    gc_init(); // Set g-code parser to default state
    fSpindle->stop();
    CoolantManager::Initialize();
    limits_init();
    Probe::Initialize();
    plan_reset(); // Clear block buffer and planner variables
    st_reset();   // Clear stepper subsystem variables

    // Sync cleared gcode and planner positions to current system position.
    plan_sync_position();
    backlash_reset_targets();
    gc_sync_position();
    report_init_message();
}

void run_once()
{
    reset_variables();
    // Start Grbl main loop. Processes program inputs and executes them.
    // This can exit on a system abort condition, in which case run_once()
    // is re-executed by an enclosing loop.
    protocol_main_loop();
}

void __attribute__((weak)) machine_init() {}

void __attribute__((weak)) display_init() {}
