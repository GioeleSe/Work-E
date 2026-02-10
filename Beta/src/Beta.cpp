#include "Beta.h"
#include "UDP_Client.h"
#include "UDP_Server.h"

Beta self;

// TODO
// loadPayload()
// dischargePayload()
//? the motor driver also has a FAULT detection pin

/*
* DC MOTOR LOGIC:

* IN1 pin | IN2 pin |   Direction
* ----------------------------------------------------
*   LOW   |   LOW   |     stop
*   HIGH  |  HIGH   |     stop
*   HIGH  |   LOW   |   forward (spin clockwise)
*   LOW   |  HIGH   |   reverse (spin anti-clockwise)
*/

// Spin the motors!
/// \param dutyCycle  Percentage of the duty cycle to be used to set PWM (0-100%)
/// \param dir  Direction of movement
//! TO BE CHANGED:
//! move only forwards and backwards, turn to change directions
//!     stop()
//!     go()
//!     turn(LEFT | RIGHT)
void move(int dutyCycle, Direction dir)
{
    int speed = (int)(MAX_SPEED * dutyCycle);

    //? if case con uno switch dentro? huh???

    switch (dir)
    {
    FORWARD:
        spinClockwise(MOTOR_1A_PIN, MOTOR_1B_PIN, speed);
        spinClockwise(MOTOR_2B_PIN, MOTOR_2B_PIN, speed);
        // self.moving = true;
        break;
    BACKWARD:
        spinAntiClockwise(MOTOR_1A_PIN, MOTOR_1B_PIN, speed);
        spinAntiClockwise(MOTOR_2B_PIN, MOTOR_2B_PIN, speed);
        // self.moving = true;
        break;
    LEFT:
        // todo turn()
        //  self.moving = true;
        break;
    RIGHT:
        // todo turn()
        //  self.moving = true;
        break;
    STOP:
        digitalWrite(ULT_PIN, LOW);
        // stop();
        break;
    default:
        //! ERROR
        //! handle it NOW!!
        break;
    }
}

void spinClockwise(int MOTOR_PIN1, int MOTOR_PIN2, int speed)
{
    digitalWrite(ULT_PIN, HIGH);
    analogWrite(MOTOR_PIN1, speed);
    digitalWrite(MOTOR_PIN2, LOW);
}

void spinAntiClockwise(int MOTOR_PIN1, int MOTOR_PIN2, int speed)
{
    digitalWrite(ULT_PIN, HIGH);
    digitalWrite(MOTOR_PIN1, LOW);
    analogWrite(MOTOR_PIN2, speed);
}

/// Stops all motors
//? set low power mode for motors -> here or in main?
void stop()
{
    analogWrite(MOTOR_1A_PIN, 0);
    analogWrite(MOTOR_1B_PIN, 0);
    analogWrite(MOTOR_2A_PIN, 0);
    analogWrite(MOTOR_2B_PIN, 0);

    // self.moving = false;
}