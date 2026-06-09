#include "Beta.h"
#include "trunk_motion.h"
extern Beta_t robot_beta;

// define the switch pin and close the trunk as initial state
void setupTrunk()
{
    logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "starting trunk setup");
    pinMode(PIN_TRUNK_LIMIT, INPUT_PULLUP);
    if(robot_beta.object_unloader){
        closeTrunk();
    }
}

// start the trunk motor forward for a fixed time of 1 second
// guarded by object_unloader property — if 0, the trunk is disabled
void openTrunk()
{
    logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "openTrunk: starting motor");
    robot_beta.trunkStartTime = millis();
    robot_beta.trunkState     = TrunkState_t::TRUNK_OPENING;
    self_motion_activate_dc_motor(MOTOR_TRUNK, Direction_t::Direction_FORWARD, 100, 500);
}

// closing procedure:
// - guard check on object_unloader property
// - check the limit switch: if already closed, set TRUNK_READY and return
// - if not closed, start moving the motor backward and set TRUNK_CLOSING
// - updateTrunk() will supervise the closing sequence against the max activation time
void closeTrunk()
{
    logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "closeTrunk: called");
    int limit = digitalRead(PIN_TRUNK_LIMIT);
    char buf[64];
    snprintf(buf, sizeof(buf), "closeTrunk: limit switch reads %d", limit);
    logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, buf);

    if (limit == HIGH)
    {
        logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "closeTrunk: trunk already closed");
        robot_beta.trunkState = TrunkState_t::TRUNK_READY;
        return;
    }
    logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "closeTrunk: trunk open, starting closing sequence");
    self_motion_activate_dc_motor(MOTOR_TRUNK, Direction_t::Direction_BACKWARD, 100, 0);
    robot_beta.trunkStartTime = millis();
    robot_beta.trunkState     = TrunkState_t::TRUNK_CLOSING;
}

// called every loop iteration to supervise trunk state transitions.
// reacts to moveTrunkCmd property: 1 = open, 0 = close, -1 = no pending command
// supervises TRUNK_OPENING timeout (1s motor + 3s total window)
// supervises TRUNK_CLOSING via limit switch and max timeout (5s)
// resets moveTrunkCmd to -1 when any sequence completes
void updateTrunk()
{
    // only dispatch new commands when the trunk is in a stable state
    if (robot_beta.moveTrunkCmd == 1 &&
        (robot_beta.trunkState == TRUNK_READY || robot_beta.trunkState == TRUNK_IDLE))
    {
        logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "updateTrunk: cmd=1 -> openTrunk()");
        openTrunk();
        oledPrint("Raising trunk");

        return;
    }
    else if (robot_beta.moveTrunkCmd == 0 &&
            (robot_beta.trunkState == TRUNK_READY || robot_beta.trunkState == TRUNK_IDLE))
    {
        logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "updateTrunk: cmd=0 -> closeTrunk()");
        closeTrunk();
        oledPrint("Closing trunk");
        return;
    }
    // opening supervision: motor timeout handles the actual stop,
    // here we just wait for the opening window to expire then return to IDLE
    if (robot_beta.trunkState == TRUNK_OPENING)
    {
        char buf[96];
        unsigned long elapsed = millis() - robot_beta.trunkStartTime;
        snprintf(buf, sizeof(buf), "updateTrunk: OPENING elapsed=%lu ms", elapsed);
        logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, buf);
        if (elapsed >= 200)
        {
            self_motion_stop_motor(Motors_MOT3);
            robot_beta.trunkState   = TRUNK_IDLE;
            robot_beta.moveTrunkCmd = -1;
            logMessage(ErrorSeverity_t::ErrorSeverity_t_MID, "updateTrunk: trunk open, returning to IDLE");
        }
        return;
    }

    // nothing to supervise if not closing
    if (robot_beta.trunkState != TRUNK_CLOSING) return;

    // closing supervision: check limit switch and max timeout
    unsigned long elapsed = millis() - robot_beta.trunkStartTime;
    int limit = digitalRead(PIN_TRUNK_LIMIT);

    if (limit == LOW)
    {
        // limit switch reached: trunk is physically closed, stop motor
        self_motion_activate_dc_motor(MOTOR_TRUNK, Direction_t::Direction_STOP, 0, 0);
        robot_beta.trunkState   = TRUNK_READY;
        robot_beta.moveTrunkCmd = -1;
        logMessage(ErrorSeverity_t::ErrorSeverity_t_MID, "updateTrunk: trunk closed and locked");
        return;
    }
    else if (elapsed >= 1000)
    {
        // max closing time exceeded without reaching the limit switch: error state
        self_motion_activate_dc_motor(MOTOR_TRUNK, Direction_t::Direction_STOP, 0, 0);
        robot_beta.trunkState   = TRUNK_ERROR;
        robot_beta.moveTrunkCmd = -1;
        logMessage(ErrorSeverity_t::ErrorSeverity_t_HIGH, "updateTrunk: closing timed out, button not reached");
    }
}