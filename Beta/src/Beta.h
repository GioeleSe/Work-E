// Header file for defining the Software/Hardware interface and controlling the robot

#ifndef BETA

#define BETA

//? include all necessary libraries to control sensors & actuators
//? define pins
//? define motor speed

enum State
{
    IDLE,
    BUSY,
    ERROR,
    DISCONNECTED,
};

enum NavigationType
{
    RADAR, // Automatic
    POSITIONING, // Manual/Debug
};

enum Direction
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
};

typedef struct Beta
{
    NavigationType navigationMode;
    State currentState;
    // Booleans to be used to determine currentState
    bool connected;
    bool moving;
} Beta;

//* move()
//* stop() -> low power mode for the motor driver (SLEEP pin)
//* loadPayload()
//* dischargePayload()
//? the motor driver also has a FAULT detection pin
//? motors are controlled using PWM: 0-100% duty cicle controlled by ui interface

#endif