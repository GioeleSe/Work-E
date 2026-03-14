

#include "fsm.h"
#include "CharlieUDP.h"
#include "tasks.h"

RobotState_t state = ROBOT_IDLE;

uint16_t lastRequestId = 0;
bool busyStarted = false;
unsigned long busyStartTime = 0;

void setupFSM() {
  state = ROBOT_IDLE;
  lastRequestId = 0;
  busyStarted = false;
  busyStartTime = 0;
}


void updateFSM(unsigned long now) {
  switch (state) {
    
    case ROBOT_IDLE:
      break;

    case ROBOT_BUSY:
      
      if (!busyStarted) {
        busyStartTime = now;
        busyStarted = true;
        Serial.println("[FSM] BUSY: task started");
      }


      if (now - busyStartTime >= 10000) {
        Serial.println("[FSM] BUSY: task completed");


        sendFeedback(lastRequestId, 0, 0, "");

        state = ROBOT_IDLE;
        busyStarted = false;
      }
      break;

    case ROBOT_ERR:

      break;
  }
}
