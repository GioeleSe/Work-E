#include "Beta.h"
#include "trunk_motion.h"

extern Beta_t robot_beta;

// define the switch pin and close the trunk
void setupTrunk()
{
    logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "starting trunk setup");
    pinMode(PIN_TRUNK_LIMIT, INPUT_PULLUP);
    closeTrunk();
}

// start the trunk motor for a fixed time of 3 seconds
void openTrunk()
{
    if (!robot_beta.object_unloader) return;

    logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "opening trunk");
    self_motion_activate_dc_motor(MOTOR_TRUNK, Direction_t::Direction_FORWARD, 75, 1000);
}

// closing procedure:
// - check the limit switch
// - if not closed, start moving the motor and update trunkStartTime and trunkState
// - updateTrunk() will check TRUNK_CLOSING state against the max activation time
void closeTrunk()
{
    if (!robot_beta.object_unloader) return;

    logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "closing trunk");
    if (digitalRead(PIN_TRUNK_LIMIT) == HIGH)
    {
        logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "trunk already closed");
        robot_beta.trunkState = TrunkState_t::TRUNK_READY;
        return;
    }

    logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "trunk open. Initializing closing sequence...");
    self_motion_activate_dc_motor(MOTOR_TRUNK, Direction_t::Direction_BACKWARD, 100, 0);
    robot_beta.trunkStartTime = millis();
    robot_beta.trunkState     = TrunkState_t::TRUNK_CLOSING;
}

// check if the trunk is closing, impose a max closing time of 5 seconds
void updateTrunk()
{
    if (robot_beta.trunkState != TRUNK_CLOSING) return;

    if (digitalRead(PIN_TRUNK_LIMIT) == HIGH)
    {
        self_motion_activate_dc_motor(MOTOR_TRUNK, Direction_t::Direction_STOP, 0, 0);
        robot_beta.trunkState = TRUNK_READY;
        logMessage(ErrorSeverity_t::ErrorSeverity_t_MID, "trunk successfully closed and locked");
        return;
    }
    else if (millis() - robot_beta.trunkStartTime >= 5000)
    {
        self_motion_activate_dc_motor(MOTOR_TRUNK, Direction_t::Direction_STOP, 0, 0);
        robot_beta.trunkState = TRUNK_ERROR;
        logMessage(ErrorSeverity_t::ErrorSeverity_t_HIGH, "trunk closing timed out, button not reached");
    }
}