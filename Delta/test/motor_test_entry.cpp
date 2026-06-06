// On-device motor test — calls main_robot motion functions directly, no UDP stack needed.
// Build and upload: pio run -e motor_test -t upload
// Open Serial at 115200 to observe steps.

#include <Arduino.h>
#include "main_robot.h"

extern SemaphoreHandle_t self_robot_state_sem;

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(200);

    self_robot_state_sem = xSemaphoreCreateMutex();

    // Wake the wheel motor driver and attach PWM channels
    pinMode(PIN_WHEELS_DRIVER_SLEEP, OUTPUT);
    digitalWrite(PIN_WHEELS_DRIVER_SLEEP, HIGH);
    ledcAttach(PIN_WHEELS_DRIVER_IN1, 1000, 8);
    ledcAttach(PIN_WHEELS_DRIVER_IN2, 1000, 8);
    ledcAttach(PIN_WHEELS_DRIVER_IN3, 1000, 8);
    ledcAttach(PIN_WHEELS_DRIVER_IN4, 1000, 8);

    self_motion_car_stop();
    Serial.println("Motor test ready — forward/stop/backward loop");
}

void loop() {
    Serial.println("Forward 1s");
    self_motion_car_proceed(Direction_FORWARD);
    delay(1000);

    Serial.println("Stop 500ms");
    self_motion_car_stop();
    delay(500);

    Serial.println("Backward 1s");
    self_motion_car_proceed(Direction_BACKWARD);
    delay(1000);

    Serial.println("Stop 500ms");
    self_motion_car_stop();
    delay(500);
}
