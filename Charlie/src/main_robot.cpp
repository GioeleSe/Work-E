// Charlie ^^ (id=1) (ip = 192.168.137.101)

#include "robot_server.h" // for Charlie server functions
#include "udp_client.h" // for sending UDP feedback to user app

#include <ESP32Servo.h> // for servo control
#include "Wire.h" // I2C communication
#include "VL53L0X.h" // ToF distance sensor
#include <Adafruit_SSD1306.h> // OLED display
#include <Adafruit_GFX.h> // OLED graphics library



// LEDs (pins)
#define LED_GREEN 14
#define LED_RED 15

// OLED (doesn't work)
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_ADDRESS 0x3C // OLED display needs an I2C address

// Hardware objects
Servo radar_servo; // Continuous rotation servo for radar sweep, attached to pin 13
VL53L0X sensor; // Time-of-Flight distance sensor
TwoWire I2C_OLED = TwoWire(0); // I2C bus 0 for OLED (SDA=33, SCL=32)
TwoWire I2C_SENSOR = TwoWire(1); // I2C bus 1 for sensor (SDA=26, SCL=27)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C_OLED, -1);



// RADAR SERVO CONFIG (continuous rotation servo)
#define SERVO_PIN               13 // Pin to which the radar servo is attached
#define SERVO_STOP_US           1500 // Microseconds for stop (neutral) position
#define SERVO_CW_US             1300 // Microseconds for clockwise rotation
#define SERVO_CCW_US            1700 // Microseconds for counterclockwise rotation
#define SERVO_DEG_PER_MS_CW     0.25f // Approximate degrees per millisecond for CW rotation
#define SERVO_DEG_PER_MS_CCW    0.18f // Approximate degrees per millisecond for CCW rotation
#define RADAR_CONE_DEG          60 // Total sweep cone in degrees
#define RADAR_STEP_DEG          15 // Step size for radar sweep in degrees
#define RADAR_SAMPLES           (RADAR_CONE_DEG / RADAR_STEP_DEG + 1) // Number of samples in a full sweep

// MOTOR CONFIG --- LEDC CHANNELS
#define LEDC_FREQ 1000 // 1000 Hz (PWM)
#define LEDC_RESOL 8 // 8 bits (0=off --- 255=max) (128=half=500Hz)


// M1 = MOTOR1 = LEFT WHEEL, M2 = MOTOR2 = RIGHT WHEEL (BOTH DRIVER1)
#define CH_M1_IN1 0 // LEDC channel for M1 IN1
#define CH_M1_IN2 1
#define CH_M2_IN1 2
#define CH_M2_IN2 3

#define M1_IN1 25
#define M1_IN2 23
#define M2_IN1 22
#define M2_IN2 21

// MOTOR 3 (BRUSH LEFT) - Driver 2
#define CH_M3_IN1 4
#define CH_M3_IN2 5
#define M3_IN1 19
#define M3_IN2 18

// MOTOR 4 (BRUSH RIGHT) - Driver 2
#define CH_M4_IN1 6
#define CH_M4_IN2 7
#define M4_IN1 17
#define M4_IN2 16

int robot_id = 1; // Charlie 1
int robot_state = RobotState_IDLE; // IDLE, BUSY, ERROR
int speed_level = 80; // default 80%, range 0-100
int feedback_level = FeedbackLevel_SILENT; // SILENT, MINIMAL, DEBUG
int debug_level = DebugLevel_OFF; // OFF, BASIC, FULL
int navigation_type = NavigationType_MANUAL; // MANUAL, CHECKPOINT, GRID, FREE_MOVE
int route_policy = RoutePolicy_SHORTEST; // SHORTEST, SAFEST, FAST
int radar_enabled = 0;
int radar_servo_angle = -1;
int screen_enabled = 1;
int lights_enabled = 0;
int brushes_enabled = 0;
int obstacle_cleaner = 1;
int object_loader = 0;
int object_unloader = 0;
int object_compacter = 0;

unsigned long green_led_off_ms = 0; // 0 = indefinite, >0 = turn off at this millis()

void self_led_green_on() {          // stays on until self_led_green_off() is called
    green_led_off_ms = 0;
    digitalWrite(LED_GREEN, HIGH);
}

void self_led_green_pulse(int ms) { // stays on for ms milliseconds then turns off
    green_led_off_ms = millis() + ms;
    digitalWrite(LED_GREEN, HIGH);
}

void self_led_green_off() {
    green_led_off_ms = 0;
    digitalWrite(LED_GREEN, LOW);
}

void self_led_green_tick() {        // call periodically to expire timed pulses
    if(green_led_off_ms > 0 && millis() >= green_led_off_ms) {
        digitalWrite(LED_GREEN, LOW);
        green_led_off_ms = 0;
    }
}

void initMotors(){
    // Initialize motor control using LEDC (PWM)
    // Note: Motors are initially stopped; they will be activated on demand
    // Each motor has two control pins (IN1, IN2) for direction and speed control via PWM.



    // setup CH for M1-IN1
    ledcSetup(CH_M1_IN1, LEDC_FREQ, LEDC_RESOL); // this channel has this freq and resol
    ledcAttachPin(M1_IN1, CH_M1_IN1); // and it is used for this pin

    // setup CH for M1-IN2
    ledcSetup(CH_M1_IN2, LEDC_FREQ, LEDC_RESOL);
    ledcAttachPin(M1_IN2, CH_M1_IN2);

    // setup CH for M2-IN1
    ledcSetup(CH_M2_IN1, LEDC_FREQ, LEDC_RESOL);
    ledcAttachPin(M2_IN1, CH_M2_IN1);

    // setup CH for M2-IN2
    ledcSetup(CH_M2_IN2, LEDC_FREQ, LEDC_RESOL);
    ledcAttachPin(M2_IN2, CH_M2_IN2);

    // brushes

    ledcSetup(CH_M3_IN1, LEDC_FREQ, LEDC_RESOL);
    ledcAttachPin(M3_IN1, CH_M3_IN1);
    ledcSetup(CH_M3_IN2, LEDC_FREQ, LEDC_RESOL);
    ledcAttachPin(M3_IN2, CH_M3_IN2);
    ledcSetup(CH_M4_IN1, LEDC_FREQ, LEDC_RESOL);
    ledcAttachPin(M4_IN1, CH_M4_IN1);
    ledcSetup(CH_M4_IN2, LEDC_FREQ, LEDC_RESOL);
    ledcAttachPin(M4_IN2, CH_M4_IN2);

    // Initialize LEDs
    pinMode(LED_GREEN, OUTPUT);   // Pin 14 as output
    pinMode(LED_RED, OUTPUT);     // Pin 15 as output
    digitalWrite(LED_GREEN, LOW); // Turn both off
    digitalWrite(LED_RED, LOW);

    // Servo — not attached at startup, activated only on demand
    
    // OLED I2C (bus 0: SDA=33, SCL=32)
    I2C_OLED.begin(33, 32);

    display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
    display.clearDisplay();
    display.display();

    // VL53L0X I2C (bus 1: SDA=26, SCL=27)
    I2C_SENSOR.begin(26, 27);
    I2C_SENSOR.setTimeOut(50); // 50ms max per I2C op — prevents permanent hang on bus corruption
    sensor.setBus(&I2C_SENSOR);
    sensor.init();
    sensor.setTimeout(500);
    sensor.startContinuous();

}

// ---- Radar servo helpers (continuous rotation) ----

void radar_servo_stop() {
    radar_servo.attach(SERVO_PIN);
    radar_servo.writeMicroseconds(SERVO_STOP_US);
}

void radar_servo_cw() {
    radar_servo.attach(SERVO_PIN);
    radar_servo.writeMicroseconds(SERVO_CW_US);
}

void radar_servo_ccw() {
    radar_servo.attach(SERVO_PIN);
    radar_servo.writeMicroseconds(SERVO_CCW_US);
}

void radar_servo_rotate_deg(float deg, bool clockwise) {
    if (clockwise) {
        radar_servo_cw();
        delay((int)(deg / SERVO_DEG_PER_MS_CW));
    } else {
        radar_servo_ccw();
        delay((int)(deg / SERVO_DEG_PER_MS_CCW));
    }
    radar_servo_stop();
    delay(50);
}

// Helper: send a single distance reading to the server
void radar_send_distance() {
    uint16_t raw = sensor.readRangeContinuousMillimeters();
    if(!sensor.timeoutOccurred() && raw < 8190) {
        JsonDocument doc;
        doc["protocol"]     = "robot-net/1.0";
        doc["robot_id"]     = robot_id;
        doc["message_type"] = (int)MessageType_t::MessageType_FEEDBACK;
        doc["request_id"]   = 0;
        doc["mode"]         = (int)MessageMode_t::MessageMode_MANUAL;
        doc["timestamp"]    = (long)time(NULL);
        doc["payload"]["event"]           = "radar_scan";
        doc["payload"]["min_distance_mm"] = raw;
        char buf[BUFFER_SIZE];
        ssize_t len = serializeJson(doc, buf);
        client_send_packet(buf, len);
    }
}

// stub — arrows removed, radar_servo_drive no longer used
void radar_servo_drive(Direction_t direction) {}

// Reinitialize I2C bus and sensor after a lockup
void radar_sensor_recover() {
    Serial.println("[RADAR] I2C lockup detected — recovering...");
    I2C_SENSOR.end();
    delay(10);
    I2C_SENSOR.begin(26, 27);
    I2C_SENSOR.setTimeOut(50);
    sensor.setBus(&I2C_SENSOR);
    sensor.init();
    sensor.setTimeout(500);
    sensor.startContinuous();
    Serial.println("[RADAR] sensor recovered.");
}

// ---- Radar FreeRTOS task ----
// This task continuously performs radar sweeps when radar_enabled is true, and sleeps otherwise.

void* radar_task(void* arg) { // 
    while(1) {
        if(radar_enabled) {
            int min_dist = 8200;
            int valid_count = 0;
            bool timed_out = false;
            unsigned long sweep_start = millis();
            const unsigned long SWEEP_TIMEOUT_MS = 5000;

            // move to left edge of cone
            radar_servo_rotate_deg(RADAR_CONE_DEG / 2, false);

            // sweep CW — servo stops between each step, then read
            for(int i = 0; i < RADAR_SAMPLES; i++) {
                if(millis() - sweep_start > SWEEP_TIMEOUT_MS) {
                    Serial.println("[RADAR] sweep timeout — aborting");
                    radar_servo_stop();
                    timed_out = true;
                    break;
                }
                uint16_t raw = sensor.readRangeContinuousMillimeters();
                if(sensor.timeoutOccurred()) {
                    // I2C likely locked up — recover and abort this sweep
                    radar_sensor_recover();
                    timed_out = true;
                    break;
                }
                if(raw < min_dist) min_dist = raw;
                valid_count++;
                if(i < RADAR_SAMPLES - 1)
                    radar_servo_rotate_deg(RADAR_STEP_DEG, true);
            }

            if(!timed_out) {
                radar_servo_rotate_deg(RADAR_CONE_DEG - RADAR_STEP_DEG, false);
            }

            if(valid_count > 0) { // send feedback only if we got at least one valid reading
                JsonDocument doc;
                doc["protocol"]     = "robot-net/1.0";
                doc["robot_id"]     = robot_id;
                doc["message_type"] = (int)MessageType_t::MessageType_FEEDBACK;
                doc["request_id"]   = 0;
                doc["mode"]         = (int)MessageMode_t::MessageMode_MANUAL;
                doc["timestamp"]    = (long)time(NULL);
                doc["payload"]["event"]           = "radar_scan_min";
                doc["payload"]["min_distance_mm"] = min_dist;
                char buf[BUFFER_SIZE];
                ssize_t len = serializeJson(doc, buf);
                client_send_packet(buf, len);
                Serial.printf("[RADAR] min distance: %d mm (%d/%d valid)\n", min_dist, valid_count, RADAR_SAMPLES);
            }

        } else {
            radar_servo.detach(); // no signal when radar is off
            self_led_green_tick();
            platform_sleep_ms(500);
        }
    }
    platform_thread_exit();
    return NULL;
}

void startRadarTask() { // call once at startup to launch the radar task
    platform_thread_t radar_tid; // task handle (not used)
    platform_thread_create(&radar_tid, radar_task, NULL, "radar_task"); 
    Serial.println("Radar task started.");
}

// motor_id (1,2), direction, speed (0-100), duration
int self_motion_activate_dc_motor(Motors_t motor_id, Direction_t direction, int speed, int duration){
    
    int PWM = (speed*255) / 100;

    if(motor_id == Motors_MOT1){ // LEFT WHEEL
        if(direction == Direction_FORWARD){
            ledcWrite(CH_M1_IN1,PWM);
            ledcWrite(CH_M1_IN2,0);
        }

        else if (direction == Direction_BACKWARD){
            ledcWrite(CH_M1_IN1,0);
            ledcWrite(CH_M1_IN2,PWM);
        }

        else if (direction == Direction_STOP){
            ledcWrite(CH_M1_IN1,0);
            ledcWrite(CH_M1_IN2,0);
        }

    }

    else if(motor_id == Motors_MOT2){ // RIGHT WHEEL
        if(direction == Direction_FORWARD){
            ledcWrite(CH_M2_IN1,PWM);
            ledcWrite(CH_M2_IN2,0);
        }

        else if (direction == Direction_BACKWARD){
            ledcWrite(CH_M2_IN1,0);
            ledcWrite(CH_M2_IN2,PWM);
        }

        else if (direction == Direction_STOP){
            ledcWrite(CH_M2_IN1,0);
            ledcWrite(CH_M2_IN2,0);
        }

    }

    else if(motor_id == Motors_MOT3){ // BRUSH LEFT
        if(direction == Direction_FORWARD){
            ledcWrite(CH_M3_IN1,PWM);
            ledcWrite(CH_M3_IN2,0);
        }
        else if (direction == Direction_BACKWARD){
            ledcWrite(CH_M3_IN1,0);
            ledcWrite(CH_M3_IN2,PWM);
        }
        else if (direction == Direction_STOP){
            ledcWrite(CH_M3_IN1,0);
            ledcWrite(CH_M3_IN2,0);
        }
    }

    else if(motor_id == Motors_MOT4){ // BRUSH RIGHT (physically inverted)
        if(direction == Direction_FORWARD){
            ledcWrite(CH_M4_IN1,0);
            ledcWrite(CH_M4_IN2,PWM);
        }
        else if (direction == Direction_BACKWARD){
            ledcWrite(CH_M4_IN1,PWM);
            ledcWrite(CH_M4_IN2,0);
        }
        else if (direction == Direction_STOP){
            ledcWrite(CH_M4_IN1,0);
            ledcWrite(CH_M4_IN2,0);
        }
    }

    return 0;
}

int self_motion_stop_motor(Motors_t motor_id){ // stop motor immediately

    if(motor_id == Motors_MOT1){
        ledcWrite(CH_M1_IN1,0);
        ledcWrite(CH_M1_IN2,0);
    }
    
    else if(motor_id == Motors_MOT2){
        ledcWrite(CH_M2_IN1,0);
        ledcWrite(CH_M2_IN2,0);
    }

    else if(motor_id == Motors_MOT3){
        ledcWrite(CH_M3_IN1,0);
        ledcWrite(CH_M3_IN2,0);
    }
    
    else if(motor_id == Motors_MOT4){
        ledcWrite(CH_M4_IN1,0);
        ledcWrite(CH_M4_IN2,0);
    }

    return 0;
}


// Higher-level movement functions
int self_motion_car_proceed(Direction_t direction){ 
    Direction_t mot1_dir = (direction == Direction_FORWARD) ? Direction_BACKWARD : Direction_FORWARD;
    self_motion_activate_dc_motor(Motors_MOT1, mot1_dir, speed_level, 0);
    self_motion_activate_dc_motor(Motors_MOT2, direction, speed_level, 0);

    return 0;
}

int self_motion_car_stop(){ 

    self_motion_stop_motor(Motors_MOT1);
    self_motion_stop_motor(Motors_MOT2);

    return 0;
}

int self_emergency_stop(){ // GUI
    robot_state = RobotState_ERR;
    self_motion_car_stop();          // stop wheels
    self_motion_stop_motor(Motors_MOT3); // stop brush left
    self_motion_stop_motor(Motors_MOT4); // stop brush right
    brushes_enabled = 0;
    radar_enabled = 0;               // stop radar sweep
    radar_servo_stop();              // stop servo
    return 0;
}

int self_motion_steer_servo(Motors_t motor_id, int angle){
    if(motor_id == Motors_MOT5) {                    // MOT5 = radar servo
        int safe_angle = constrain(angle, 0, 180);
        radar_servo.attach(13);                      // activate only when needed
        radar_servo.write(safe_angle);
        delay(500);                                  // wait for servo to reach position
        radar_servo.detach();                        // deactivate — no signal, no noise
        radar_servo_angle = safe_angle;              // save current angle
        Serial.printf("Servo moved to %d°\n", radar_servo_angle);
        return 0;
    }
    return -1;
}

int self_motion_car_rotate(Direction_t direction){
    // we rotate by driving wheels in opposite directions at same speed
    if(direction == Direction_LEFT) {
        // FORWARD for both since motors are mounted in opposite directions
        self_motion_activate_dc_motor(Motors_MOT1, Direction_FORWARD, speed_level, 0);
        self_motion_activate_dc_motor(Motors_MOT2, Direction_FORWARD, speed_level, 0);
    }
    else if(direction == Direction_RIGHT) {
        self_motion_activate_dc_motor(Motors_MOT1, Direction_BACKWARD, speed_level, 0);
        self_motion_activate_dc_motor(Motors_MOT2, Direction_BACKWARD, speed_level, 0);
    }

    return 0;
}

int self_hard_reset(){
    self_motion_car_stop();
    self_motion_stop_motor(Motors_MOT3);
    self_motion_stop_motor(Motors_MOT4);
    brushes_enabled = 0;
    radar_enabled = 0;
    radar_servo_stop();
    robot_state = RobotState_IDLE;
    return 0;
}

int self_soft_reset(){
    robot_state = RobotState_IDLE;
    return 0;
}

// prop getters

int self_prop_get_robot_id(){
    return robot_id;
}

int self_prop_get_robot_state(){
    return robot_state;
}

int self_prop_get_speed(){
    return speed_level;
}

int self_prop_get_feedback(){
    return feedback_level;
}

int self_prop_get_debug(){
    return debug_level;
}

int self_prop_get_navigation_type(){
    return navigation_type;
}

int self_prop_get_route_policy() {
    return route_policy;
}

int self_prop_get_radar() {
    return radar_enabled;
}

int self_prop_get_screen() {
    return screen_enabled;
}

int self_prop_get_lights() {
    return lights_enabled;
}

int self_prop_get_brushes() {
    return brushes_enabled;
}

int self_prop_get_obstacle_cleaner() {
    return obstacle_cleaner;
}

int self_prop_get_object_loader() {
    return object_loader;
}

int self_prop_get_object_unloader() {
    return object_unloader;
}

int self_prop_get_object_compacter() {
    return object_compacter;
}

// prop setters

int self_prop_set_speed(int new_value){
    speed_level = new_value;
    return 0;
}

int self_prop_set_feedback(int new_value){
    feedback_level = new_value;
    return 0;
}

int self_prop_set_debug(int new_value){
    debug_level = new_value;
    return 0;
}

int self_prop_set_navigation_type(int new_value){
    navigation_type = new_value;
    return 0;
}

int self_prop_set_route_policy(int new_value){
    route_policy = new_value;
    return 0;
}

int self_prop_set_radar(int new_value){
    radar_enabled = new_value;
    return 0;
}

int self_prop_set_screen(int new_value){
    screen_enabled = new_value;
    return 0;
}

int self_prop_set_obstacle_cleaner(int new_value){
    obstacle_cleaner = new_value;
    return 0;
}

int self_prop_set_object_loader(int new_value){
    object_loader = new_value;
    return 0;
}

int self_prop_set_object_unloader(int new_value){
    object_unloader = new_value;
    return 0;
}

int self_prop_set_object_compacter(int new_value){
    object_compacter = new_value;
    return 0;
}

int self_prop_set_lights(int new_value){ // 0 = off, 1 = on — controls RED LED only
    lights_enabled = new_value;
    digitalWrite(LED_RED, new_value ? HIGH : LOW);
    return 0;
}



// brushes, we activate both motors in opposite directions
int self_prop_set_brushes(int new_value){ // 0 = off, 1 = on
    brushes_enabled = new_value; // save state (for feedback)
    if(new_value){
        // self_motion_activate_dc_motor(Motor, Direction, Speed, Duration)
        self_motion_activate_dc_motor(Motors_MOT3, Direction_FORWARD, 100, 0);
        self_motion_activate_dc_motor(Motors_MOT4, Direction_FORWARD, 100, 0);
    } else {
        self_motion_stop_motor(Motors_MOT3);
        self_motion_stop_motor(Motors_MOT4);
    }
    return 0;
}