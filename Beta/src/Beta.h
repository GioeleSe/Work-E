// Header file for defining the Software/Hardware interface and controlling the robot

#ifndef BETA

#define BETA

// Control pins for motors A-B
//? placeholders
#define MOTOR_A1_PIN 0
#define MOTOR_A2_PIN 1
#define MOTOR_B1_PIN 2
#define MOTOR_B2_PIN 3
#define ULT_PIN 4 // To enable sleep mode

#define MAX_SPEED 2.55

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
    STOP,
};

typedef struct Beta
{
    NavigationType navigationMode;
    State currentState;
    // Booleans to be used to determine currentState
    bool connected;
    bool moving;
} Beta;

void move(int dutyCycle, Direction dir);
void spinClockwise(int MOTOR_PIN1, int MOTOR_PIN2, int speed);
void spinAntiClockwise(int MOTOR_PIN1, int MOTOR_PIN2, int speed);
void stop();

#endif