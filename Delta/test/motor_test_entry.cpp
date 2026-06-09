#include <Arduino.h>
#include <ESP32Servo.h>
#include "main_robot.h"
#include "soc/rtc_cntl_reg.h"

extern SemaphoreHandle_t self_robot_state_sem;
extern SemaphoreHandle_t self_robot_prop_setter;
extern Servo clawServo;
extern Servo leverServo;

void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disabled for calibration only
    Serial.begin(SERIAL_BAUD);
    delay(200);

    self_robot_state_sem  = xSemaphoreCreateMutex();
    self_robot_prop_setter = xSemaphoreCreateMutex();

    ESP32PWM::allocateTimer(0); // claw  → timer 0 ch 0 (50 Hz)
    ESP32PWM::allocateTimer(0); // lever → timer 0 ch 1 (50 Hz, same timer independent duty)
    ESP32PWM::allocateTimer(3); // radar → timer 3 ch 6 (50 Hz); wheels stay on timers 1–2
    clawServo.setPeriodHertz(50);  clawServo.attach(PIN_CLAW_SERVO, CLAW_SERVO_MIN_US, CLAW_SERVO_MAX_US);
    leverServo.setPeriodHertz(50); leverServo.attach(PIN_LEVER_SERVO, LEVER_SERVO_MIN_US, LEVER_SERVO_MAX_US);
    ledcSetup(CH_WHEEL_R_IN1, 1000,  8); ledcAttachPin(PIN_WHEELS_DRIVER_IN1, CH_WHEEL_R_IN1);
    ledcSetup(CH_WHEEL_R_IN2, 1000,  8); ledcAttachPin(PIN_WHEELS_DRIVER_IN2, CH_WHEEL_R_IN2);
    ledcSetup(CH_WHEEL_L_IN3, 1000,  8); ledcAttachPin(PIN_WHEELS_DRIVER_IN3, CH_WHEEL_L_IN3);
    ledcSetup(CH_WHEEL_L_IN4, 1000,  8); ledcAttachPin(PIN_WHEELS_DRIVER_IN4, CH_WHEEL_L_IN4);

    pinMode(PIN_WHEELS_DRIVER_SLEEP, OUTPUT);
    digitalWrite(PIN_WHEELS_DRIVER_SLEEP, HIGH);

    clawServo.writeMicroseconds(CLAW_SERVO_MIN_US + (int)((long)CLAW_ANGLE_MAX  * (CLAW_SERVO_MAX_US  - CLAW_SERVO_MIN_US)  / 180));
    leverServo.writeMicroseconds(LEVER_SERVO_MIN_US + (int)((long)LEVER_ANGLE_MIN * (LEVER_SERVO_MAX_US - LEVER_SERVO_MIN_US) / 180));
    delay(600);

    if (self_display_init() == 0) {
        self_display_show("TEST MODE", "");
    } else {
        Serial.println("[OLED] init failed");
    }

    if (self_radar_init() == 0) {
        Serial.println("[RADAR] VL53L0X ready");
    } else {
        Serial.println("[RADAR] VL53L0X init failed");
    }

    Serial.println("Commands:");
    Serial.println("  f : forward         b : backward      x : stop wheels");
    Serial.println("  z : turn left       v : turn right");
    Serial.println("  q : right forward   a : right backward");
    Serial.println("  e : left forward    d : left backward");
    Serial.println("  u : claw open       i : claw close");
    Serial.println("  n : lever up (step) m : lever down (step)");
    Serial.println("  c : claw sweep      l : lever sweep");
    Serial.println("  r : radar scan      p : radar single read");
    Serial.println("  j : radar left      k : radar right     o : radar center");
}

void sweep(Servo& srv, int from, int to, int step_ms) {
    int step = (to > from) ? 1 : -1;
    for (int a = from; a != to + step; a += step) {
        srv.write(a);
        Serial.print("angle: "); Serial.println(a);
        delay(step_ms);
    }
}

void loop() {
    static int radar_angle = (RADAR_ANGLE_MIN + RADAR_ANGLE_MAX) / 2;
    static int lever_angle = LEVER_ANGLE_MIN;
    static int claw_angle  = CLAW_ANGLE_MAX;
    static const int LEVER_STEP = 5;
    static const int CLAW_STEP  = 5;

    if (!Serial.available()) { delay(50); return; }

    char cmd = Serial.read();
    switch (cmd) {
        case 'f':
            Serial.println("Wheels: forward");
            ledcWrite(CH_WHEEL_R_IN1, 180); ledcWrite(CH_WHEEL_R_IN2, 0);
            ledcWrite(CH_WHEEL_L_IN3, 0);   ledcWrite(CH_WHEEL_L_IN4, 180);
            break;
        case 'b':
            Serial.println("Wheels: backward");
            ledcWrite(CH_WHEEL_R_IN1, 0);   ledcWrite(CH_WHEEL_R_IN2, 180);
            ledcWrite(CH_WHEEL_L_IN3, 180); ledcWrite(CH_WHEEL_L_IN4, 0);
            break;
        case 'x':
            Serial.println("Wheels: stop");
            ledcWrite(CH_WHEEL_R_IN1, 0); ledcWrite(CH_WHEEL_R_IN2, 0);
            ledcWrite(CH_WHEEL_L_IN3, 0); ledcWrite(CH_WHEEL_L_IN4, 0);
            break;
        case 'z':
            Serial.println("Wheels: turn left");
            ledcWrite(CH_WHEEL_R_IN1, 180); ledcWrite(CH_WHEEL_R_IN2, 0);
            ledcWrite(CH_WHEEL_L_IN3, 180); ledcWrite(CH_WHEEL_L_IN4, 0);
            break;
        case 'v':
            Serial.println("Wheels: turn right");
            ledcWrite(CH_WHEEL_R_IN1, 0); ledcWrite(CH_WHEEL_R_IN2, 180);
            ledcWrite(CH_WHEEL_L_IN3, 0); ledcWrite(CH_WHEEL_L_IN4, 180);
            break;
        case 'q':
            Serial.println("Right wheel: forward");
            ledcWrite(CH_WHEEL_R_IN1, 180); ledcWrite(CH_WHEEL_R_IN2, 0);
            break;
        case 'a':
            Serial.println("Right wheel: backward");
            ledcWrite(CH_WHEEL_R_IN1, 0); ledcWrite(CH_WHEEL_R_IN2, 180);
            break;
        case 'e':
            Serial.println("Left wheel: forward");
            ledcWrite(CH_WHEEL_L_IN3, 0); ledcWrite(CH_WHEEL_L_IN4, 180);
            break;
        case 'd':
            Serial.println("Left wheel: backward");
            ledcWrite(CH_WHEEL_L_IN3, 180); ledcWrite(CH_WHEEL_L_IN4, 0);
            break;
        case 'u':
            claw_angle = max(claw_angle - CLAW_STEP, CLAW_ANGLE_MIN);
            self_motion_open_claw(claw_angle);
            Serial.print("Claw: angle="); Serial.println(claw_angle);
            break;
        case 'i':
            claw_angle = min(claw_angle + CLAW_STEP, CLAW_ANGLE_MAX);
            self_motion_open_claw(claw_angle);
            Serial.print("Claw: angle="); Serial.println(claw_angle);
            break;
        case 'n':
            lever_angle = max(lever_angle - LEVER_STEP, LEVER_ANGLE_MIN);
            self_motion_move_lever(lever_angle);
            Serial.print("Lever: angle="); Serial.println(lever_angle);
            break;
        case 'm':
            lever_angle = min(lever_angle + LEVER_STEP, LEVER_ANGLE_MAX);
            self_motion_move_lever(lever_angle);
            Serial.print("Lever: angle="); Serial.println(lever_angle);
            break;
        case 'c':
            Serial.println("--- Claw: MIN to MAX ---");
            sweep(clawServo, CLAW_ANGLE_MIN, CLAW_ANGLE_MAX, 30);
            delay(500);
            Serial.println("--- Claw: MAX to MIN ---");
            sweep(clawServo, CLAW_ANGLE_MAX, CLAW_ANGLE_MIN, 30);
            break;
        case 'l':
            Serial.println("--- Lever: MIN to MAX ---");
            sweep(leverServo, LEVER_ANGLE_MIN, LEVER_ANGLE_MAX, 30);
            delay(500);
            Serial.println("--- Lever: MAX to MIN ---");
            sweep(leverServo, LEVER_ANGLE_MAX, LEVER_ANGLE_MIN, 30);
            break;
        case 'r': {
            Serial.println("--- Radar: full scan ---");
            RadarReading_t readings[RADAR_MAX_READINGS];
            int n = self_radar_scan(readings, RADAR_MAX_READINGS);
            if (n < 0) {
                Serial.println("[RADAR] scan failed");
            } else {
                for (int i = 0; i < n; i++) {
                    Serial.print("  angle="); Serial.print(readings[i].angle);
                    Serial.print("  dist=");  Serial.print(readings[i].distance_mm);
                    Serial.println(" mm");
                }
                Serial.println("--- scan done ---");
            }
            break;
        }
        case 'p': {
            int dist = self_radar_read_distance();
            Serial.print("[RADAR] dist="); Serial.print(dist); Serial.println(" mm");
            break;
        }
        case 'j':
            radar_angle = max(radar_angle - RADAR_SCAN_STEP, RADAR_ANGLE_MIN);
            self_motion_steer_servo(Motors_MOT3, radar_angle);
            Serial.print("Radar: angle="); Serial.println(radar_angle);
            break;
        case 'k':
            radar_angle = min(radar_angle + RADAR_SCAN_STEP, RADAR_ANGLE_MAX);
            self_motion_steer_servo(Motors_MOT3, radar_angle);
            Serial.print("Radar: angle="); Serial.println(radar_angle);
            break;
        case 'o':
            radar_angle = (RADAR_ANGLE_MIN + RADAR_ANGLE_MAX) / 2;
            self_motion_steer_servo(Motors_MOT3, radar_angle);
            Serial.print("Radar: angle="); Serial.println(radar_angle);
            break;
        default:
            break;
    }
}
