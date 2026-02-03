/* Delta, The Robot Who Lifts 
- include statements
- setup function -> initialize communication and send first heartbeats
- loop function -> receive commands and navigation data, send feedback

Robot receives commands and puts them in a producer queue, then consumes them. 
1,2,3 are put in queue, 4,5 are handled immediately.
Base station can send commands:
 1. move to destination coordinate
 2. motor control -> to control specific robot functions
 3. set_config (low level motor params)
 4. emergency stop
 5. reset

 Robot can send message:
 - heartbeat 
 - event -> obstacle detected/cleared, load collected, destination reached
 - feedback -> delaying execution because of obstacle, or need help with load
 - error
*/

#include "config.h"
#include "communication.h"
#include "navigation.h"
#include "motor_control.h"

enum RobotState {
    IDLE, // send heartbeats, receive commands
    MOVING, // respond to movement commands
    EXECUTING, // performing task e.g. lift block
    ERROR, // something went wrong
};

RobotState current_state = IDLE;
unsigned long last_heartbeat_time = 0; // timer to count up
const unsigned long HEARTBEAT_INTERVAL = 1000; 


void initCommunication() {

}
void sendHeartbeat() {

}

void initMotors() {

}

void initNavigation() {

}

void setup() {
    //init communication
    initCommunication();
    sendHeartbeat();
    //init motors
    initMotors();
    //init navigation
    initNavigation();

}

void receiveCommand() {

}
void validateCommand() {

}
void updateNavigation() {

}
void sendFeedback(RobotState state) {

}
const int COMMAND_QUEUE_SIZE = 10;
CommandType commandQueue[COMMAND_QUEUE_SIZE];
void loop() {
    // Send heartbeat at regular intervals
    if (last_heartbeat_time >= HEARTBEAT_INTERVAL) {
        sendHeartbeat();
        last_heartbeat_time = 0;
    }

    // Check for incoming commands
    if (commandQueue.length() > 0) {
        //handle command: put in queue, or process immediately if reset or stop. 
        handleCommand();
    }


    sendFeedback(current_state);
}