/*Robot receives commands and puts them in a producer queue, then consumes them. 
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
/*
The communication module is resposible for the handle command function, 
and the main .ino delegates to it the task of sending back responses. 
*/  
enum CommandType {
    MOVE,
    MOTOR_CONTROL,
    SET_CONFIG,
    EMERGENCY_STOP,
    RESET
};

enum RobotResponse {
    HEARTBEAT,
    EVENT,
    FEEDBACK,
    ERROR
};

void handleCommand(CommandType cmd) {
    switch(cmd) {
        case MOVE:
            // enqueue move command
            break;
        case MOTOR_CONTROL:
            // enqueue motor control command
            break;
        case SET_CONFIG:
            // enqueue set config command
            break;
        case EMERGENCY_STOP:
            // handle immediately
            break;
        case RESET:
            // handle immediately
            break;
        default:
            // unknown command
            break;
    }
}