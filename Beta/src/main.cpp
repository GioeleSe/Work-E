#include <Arduino.h>
#include "Beta.h"
#include "UDP_Client.h"
#include "UDP_Server.h"

extern Beta_t robot_beta;

void setup()
{
    Serial.begin(115200);
    setupMotors();
    setupTrunk();
    // setupRadar();
    // setupOled();
    setupWiFi();
    RobotStartServer();
}

void loop()
{
    checkMotorTimeouts();
    updateTrunk();
    // tickRadarScan();

    if (robot_beta.trunkState == TRUNK_ERROR) {
        // check how to signal the error and how to behave
        // logMessage(ErrorSeverity_t::ErrorSeverity_t_HIGH ,"trunk critical error. Shutting down functionality");
        // robot_beta.object_unloader = 0; // will be cleared only with an hard reset (expecting a manual fix)
        // robot_beta.robot_state = RobotState_t::RobotState_ERR;
    }
    if(robot_beta.radar.obstacleDetected){
        // check if here is needed to be managed or only inside the move car stuff (maybe set a flag like robot_beta.canCarProceed)
        logMessage(ErrorSeverity_t::ErrorSeverity_t_HIGH ,"radar detected an obstacle. Going to prevent car from moving in direction Direction_FORWARD until the road is clear");
        robot_beta.robot_state = RobotState_t::RobotState_ERR;
    }
}