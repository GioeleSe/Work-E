#include "Beta.h"
#include "dcmotor_motion.h"
#include "radar_motion.h"   // for SERVO_STOP_US

extern Beta_t robot_beta;
void setupMotors()
{
    logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "starting motors setup");

    // ----------------------------
    // 1. Put motor drivers in a known SAFE state first
    // (Most drivers: HIGH = enabled, LOW = sleep → VERIFY YOUR HW)
    // ----------------------------
    pinMode(PIN_DRIVER_1_SLEEP, OUTPUT);
    pinMode(PIN_DRIVER_2_SLEEP, OUTPUT);

    digitalWrite(PIN_DRIVER_1_SLEEP, HIGH);
    digitalWrite(PIN_DRIVER_2_SLEEP, HIGH);

    delay(10); // allow driver to settle

    bool ok = true; // attach LEDC PWM pins (ESP32 core v3+ API)
    ok &= ledcAttach(PIN_DRIVER_1_MOTOR_1A, DRIVER_PWM_FREQ, DRIVER_PWM_RES);
    ok &= ledcAttach(PIN_DRIVER_1_MOTOR_1B, DRIVER_PWM_FREQ, DRIVER_PWM_RES);
    ok &= ledcAttach(PIN_DRIVER_1_MOTOR_2A, DRIVER_PWM_FREQ, DRIVER_PWM_RES);
    ok &= ledcAttach(PIN_DRIVER_1_MOTOR_2B, DRIVER_PWM_FREQ, DRIVER_PWM_RES);
    ok &= ledcAttach(PIN_DRIVER_2_MOTOR_1A, DRIVER_PWM_FREQ, DRIVER_PWM_RES);
    ok &= ledcAttach(PIN_DRIVER_2_MOTOR_1B, DRIVER_PWM_FREQ, DRIVER_PWM_RES);

    if (!ok)
    {
        logMessage(ErrorSeverity_t::ErrorSeverity_t_HIGH, "LED PWM attach failed!");
        return;
    }

    // ----------------------------
    // 3. Force all outputs to a safe stopped state
    // ----------------------------
    ledcWrite(PIN_DRIVER_1_MOTOR_1A, 0);
    ledcWrite(PIN_DRIVER_1_MOTOR_1B, 0);
    ledcWrite(PIN_DRIVER_1_MOTOR_2A, 0);
    ledcWrite(PIN_DRIVER_1_MOTOR_2B, 0);
    ledcWrite(PIN_DRIVER_2_MOTOR_1A, 0);
    ledcWrite(PIN_DRIVER_2_MOTOR_1B, 0);

    delay(5); // prevents startup glitching

    logMessage(ErrorSeverity_t::ErrorSeverity_t_MID, "motors setup done correctly");
}
// check if an active motor has reached the required timeout
void checkMotorTimeouts()
{
    unsigned long currentTime = millis();
    for (int i = 1; i < NUM_MOTORS; i++)
    {
        if (motorTimeouts[i].isActive){
            if (robot_beta.isRequiredStop || (currentTime >= motorTimeouts[i].stopTime)) // target time achieved, stop the motor
            {
                if (i == MOTOR_RADAR)
                {
                    robot_beta.radar.motorServo.writeMicroseconds(SERVO_STOP_US);
                }
                else
                {
                    ledcWrite(motor_pins[i].pinA, 0); // stop command for other DRV8833 motors
                    ledcWrite(motor_pins[i].pinB, 0);
                }
                motorTimeouts[i].isActive = false;
                if(robot_beta.isRequiredStop){
                    logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "motor stopped by emergency button");
                }else{
                    // logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "motor timed out and stopped automatically");
                }
                    
            }
        }
    }
}
// reusable sanity check for motor_id
int parseMotorID(Motors_t motor_id)
{
    if (motor_id < 0 || motor_id > NUM_MOTORS)
    {
        logMessage(ErrorSeverity_t::ErrorSeverity_t_HIGH, "out of range motor id requested");
        return -1;
    }
    else if (motor_pins[motor_id].pinA == 0 && motor_pins[motor_id].pinB == 0)
    {
        logMessage(ErrorSeverity_t::ErrorSeverity_t_MID, "requested motor has no DC pin assignment");
        return -2;
    }
    else
    {
        return 0;
    }
}