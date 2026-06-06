// On-device servo test — calls main_robot motion functions directly, no UDP stack needed.
// Build and upload: pio run -e motor_test -t upload
// Open Serial at 115200 to observe steps.

#include <Arduino.h>
#include "main_robot.h"

extern SemaphoreHandle_t self_robot_state_sem;

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(200);

    self_robot_state_sem = xSemaphoreCreateMutex();

    ledcSetup(CH_CLAW_SERVO,  50, 16); ledcAttachPin(PIN_CLAW_SERVO,  CH_CLAW_SERVO);
    ledcSetup(CH_LEVER_SERVO, 50, 16); ledcAttachPin(PIN_LEVER_SERVO, CH_LEVER_SERVO);

    Serial.println("Servo test ready — claw and lever sweep");
}

void loop() {
    Serial.println("Claw -> MIN");
    self_motion_open_claw(CLAW_ANGLE_MIN);
    delay(1000);

    Serial.println("Claw -> MAX");
    self_motion_open_claw(CLAW_ANGLE_MAX);
    delay(1000);

    Serial.println("Lever -> MIN");
    self_motion_move_lever(LEVER_ANGLE_MIN);
    delay(1000);

    Serial.println("Lever -> MAX");
    self_motion_move_lever(LEVER_ANGLE_MAX);
    delay(1000);
}
