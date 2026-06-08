#include "Beta.h"
#include "radar_motion.h"

extern Beta_t robot_beta;
extern Motor_Timeout_t motorTimeouts[5];

void setupRadar()
{
    robot_beta.radar.currentRadarAngle = 0;
    robot_beta.radar.lastMinDistance = -1.0f;
    robot_beta.radar.obstacleDetected = false;
    robot_beta.radar.scanInProgress = false;
    robot_beta.radar.scanStep = 0;
    robot_beta.radar.scanSum = 0.0f;
    robot_beta.radar.scanValidCount = 0;

    Wire.begin(PIN_RADAR_SDA, PIN_RADAR_SCL);
    internalSetupServo();
    internalSetupDistanceSensor();

}
void internalSetupServo()
{
    ledcDetach(PIN_SERVO_CONTROL);
    delay(10);
    robot_beta.radar.motorServo.setPeriodHertz(50);
    if (robot_beta.radar.motorServo.attach(PIN_SERVO_CONTROL) < 0)
    {
        logMessage(ErrorSeverity_t::ErrorSeverity_t_HIGH, "servo attachment failed! No hardware timers available.");
        robot_beta.radar_prop = 0;
    }
    else
    {
        logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "servo hardware attached successfully");
    }
}
void internalSetupDistanceSensor()
{
    robot_beta.radar.distanceSensor.setTimeout(500);
    if (!robot_beta.radar.distanceSensor.init())
    {
        logMessage(ErrorSeverity_t::ErrorSeverity_t_HIGH, "VL53L0X not found — check wiring!");
        robot_beta.radar_prop = 0;
    }
    else
    {
        robot_beta.radar.distanceSensor.startContinuous();
        logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "VL53L0X initialized successfully");
    }
}
// Moves the radar servo toward target_angle using microsecond pulse control.
// Motion is non-blocking: duration is computed from angle delta and handed
// off to the motor timeout system, which stops the servo when time elapses.
void moveRadarToAngle(int target_angle)
{
    target_angle = constrain(target_angle, 30, 150);
    int degrees_to_move = target_angle - robot_beta.radar.currentRadarAngle;
    if (degrees_to_move == 0) return;

    unsigned long duration;

    if (degrees_to_move > 0)
    {
        robot_beta.radar.motorServo.writeMicroseconds(SERVO_CW_US);
        duration = (unsigned long)(abs(degrees_to_move) / SERVO_DEG_PER_MS_CW);
    }
    else
    {
        robot_beta.radar.motorServo.writeMicroseconds(SERVO_CCW_US);
        duration = (unsigned long)(abs(degrees_to_move) / SERVO_DEG_PER_MS_CCW);
    }

    robot_beta.radar.currentRadarAngle = target_angle;
    motorTimeouts[MOTOR_RADAR].isActive = true;
    motorTimeouts[MOTOR_RADAR].stopTime = millis() + duration;

    logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "moving radar to specified angle");
}
// non-blocking radar scan tick. Called in the main loop.
// updates here the distance readings when the servo is not moving, compute the results at the end.
// The final output is the radar variable obstacleDetected set to true or false
void tickRadarScan()
{
    // Sweeps the frontal cone in steps, accumulates distance readings,
    // and updates lastMinDistance and obstacleDetected when the sweep completes.
    if ((motorTimeouts[MOTOR_RADAR].isActive) || (!(robot_beta.radar.motorServo.attached())) || (!robot_beta.radar_prop)) return;
    

    Radar_t &r = robot_beta.radar;

    switch (r.scanStep)
    {
        case 0: // init: reset min value and move to left edge
            r.lastMinDistance = 2000;
            r.scanValidCount = 0;
            moveRadarToAngle(constrain(r.currentRadarAngle - RADAR_CONE_DEG / 2, 30, 150));
            r.scanStep = 1;
            break;

        default: // steps 1..RADAR_SAMPLES: read then advance
        {
            uint16_t raw = r.distanceSensor.readRangeSingleMillimeters();
            if (!r.distanceSensor.timeoutOccurred())
            {
                r.lastMinDistance = (raw<r.lastMinDistance)?raw:r.lastMinDistance;
                r.scanValidCount++;
            }

            if (r.scanStep < RADAR_SAMPLES)
            {
                int nextAngle = constrain(
                    (r.currentRadarAngle - RADAR_CONE_DEG / 2) + (r.scanStep * RADAR_STEP_DEG),
                    30, 150);
                moveRadarToAngle(nextAngle);
                r.scanStep++;
            }
            else // done
            {
                if (r.scanValidCount > 0)
                {
                    r.obstacleDetected = (r.lastMinDistance < RADAR_OBSTACLE_MM);
                    if (r.obstacleDetected)
                        logMessage(ErrorSeverity_t::ErrorSeverity_t_HIGH, "obstacle detected within threshold!");
                }
                else
                    logMessage(ErrorSeverity_t::ErrorSeverity_t_HIGH, "radar scan: no valid readings");

                r.scanStep = 0; // reset for next sweep
            }
            break;
        }
    }
}