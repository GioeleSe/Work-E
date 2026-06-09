#include <Arduino.h>
#include "Beta.h"
#include "UDP_Client.h"
#include "UDP_Server.h"

extern Beta_t robot_beta;

void serverTask(void* arg)
{
    RobotStartServer();
    vTaskDelete(NULL);
}

void motorTimeoutTask(void* arg)
{
    while (true)
    {
        checkMotorTimeouts();
        vTaskDelay(pdMS_TO_TICKS(10)); // check every 10ms
    }
}

void trunkTask(void* arg)
{
    while (true)
    {
        updateTrunk();
        vTaskDelay(pdMS_TO_TICKS(100)); // trunk doesn't need fast polling
    }
}

void radarTask(void* arg)
{
    const unsigned long SCAN_INTERVAL_MS = 5000;
    unsigned long lastScanTime = 0;
    char print_buffer[32];

    while (true)
    {
        unsigned long now = millis();

        // only start a new scan if enough time has passed since the last one
        if (now - lastScanTime >= SCAN_INTERVAL_MS)
        {
            tickRadarScan();

            // update timestamp only when a full scan completes (scanStep resets to 0)
            if (robot_beta.radar.scanStep == 0){
                lastScanTime = millis();
                oledPrint("BETA - initializing");
                snprintf(print_buffer, sizeof(print_buffer), "BETA - Dist: %.2f", robot_beta.radar.lastMinDistance);
                robot_beta.radar.lastMinDistance;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void buzzerTask(void* arg)
{
    while (true)
    {
        updateBuzzer();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void stateMonitorTask(void* arg)
{
    while (true)
    {
        if (robot_beta.trunkState == TRUNK_ERROR)
        {
            // check how to signal the error and how to behave
            // logMessage(ErrorSeverity_t::ErrorSeverity_t_HIGH, "trunk critical error. Shutting down functionality");
            // robot_beta.object_unloader = 0;
        }

        if (robot_beta.radar.obstacleDetected)
        {
            // check if here is needed to be managed or only inside the move car stuff
            logMessage(ErrorSeverity_t::ErrorSeverity_t_HIGH, "radar detected an obstacle. Going to prevent car from moving in direction Direction_FORWARD until the road is clear");
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void setup()
{
    Serial.begin(115200);
    setupRadar();   // pwm channels issues solved here. servo first — claims channel 0 for pin 13
    setupMotors();  // motors get channels 1-6
    setupTrunk();
    setupBuzzer();
    setupWiFi();
    setupOled();

    oledPrint("Initializing tasks");

    // core 0 - networking
    xTaskCreatePinnedToCore(serverTask,       "server",   16384, NULL, 5, NULL, 0); // JSON + UDP

    // core 1 - robot control
    xTaskCreatePinnedToCore(motorTimeoutTask, "motors",    2048, NULL, 4, NULL, 1); // simple, keep small
    xTaskCreatePinnedToCore(trunkTask,        "trunk",     4096, NULL, 3, NULL, 1); // logMessage + snprintf
    xTaskCreatePinnedToCore(radarTask,        "radar",     8192, NULL, 3, NULL, 1); // JSON + I2C
    xTaskCreatePinnedToCore(buzzerTask,       "buzzer",    2048, NULL, 2, NULL, 1); // simple
    xTaskCreatePinnedToCore(stateMonitorTask, "monitor",   4096, NULL, 2, NULL, 1); // logMessage
    
    oledPrint("Setup done");
}

void loop()
{
    vTaskDelay(pdMS_TO_TICKS(1000)); // loop() does nothing, all work is in tasks
}