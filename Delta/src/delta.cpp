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



RobotState current_state = IDLE;

void initMotors();

//void initNavigation();

#ifndef EXCLUDE_FROM_MOTOR_TEST
void setup() {
    //init communication
    sendHeartbeat();
    //init motors
    initMotors();
    //init navigation
   // initNavigation();

}

void executeSetConfig(const SetConfigPayload& scp){
  // for now just print the params, in the future this will update actual configs
  printf("Executing set config: prop=%d, new_value=%s\n", scp.prop, (const char*)scp.new_value);
  return;
}
void executeGetConfig(const GetConfigPayload& gcp){
  // for now just print the params, in the future this will retrieve actual config values and send feedback
  printf("Executing get config: prop=%d\n", gcp.prop);
  return;
}
void setRobotState(RobotState new_state){
  current_state = new_state;
  // optionally send event feedback on state change
}
const int COMMAND_QUEUE_SIZE = 10;
CommandType commandQueue[COMMAND_QUEUE_SIZE];

void commsLoop(); //calls handlePacket and sendHeartbeat
#endif