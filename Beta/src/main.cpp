#include <Arduino.h>
#include "Beta.h"
#include "UDP_Client.h"
#include "UDP_Server.h"

extern Beta_t robot_beta;
SemaphoreHandle_t oledMutex;

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
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void trunkTask(void* arg)
{
    while (true)
    {
        updateTrunk();
        vTaskDelay(pdMS_TO_TICKS(100));
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

        if (now - lastScanTime >= SCAN_INTERVAL_MS)
        {
            tickRadarScan();

            if (robot_beta.radar.scanStep == 0)
            {
                lastScanTime = millis();

                snprintf(print_buffer, sizeof(print_buffer),
                         "Dist: %.2f",
                         robot_beta.radar.lastMinDistance);

                oledPrint("Radar scanning...");
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
        if (robot_beta.radar.obstacleDetected)
        {
            logMessage(
                ErrorSeverity_t::ErrorSeverity_t_HIGH,
                "Obstacle detected"
            );

            oledPrint("OBSTACLE!");
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void setup()
{
    Serial.begin(115200);

    // Create mutex FIRST
    oledMutex = xSemaphoreCreateMutex();

    setupRadar();
    setupMotors();
    setupTrunk();
    setupBuzzer();
    setupOled();
    setupWiFi();

    oledPrint("Initializing tasks");
    logMessage(ErrorSeverity_t::ErrorSeverity_t_HIGH, "Initializing tasks");

    xTaskCreatePinnedToCore(serverTask,       "server", 16384, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(motorTimeoutTask, "motors", 2048, NULL, 4, NULL, 1);
    xTaskCreatePinnedToCore(trunkTask,        "trunk",  4096, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(radarTask,        "radar",  8192, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(buzzerTask,       "buzzer", 2048, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(stateMonitorTask, "monitor",4096, NULL, 2, NULL, 1);

    oledPrint("Setup done");
    logMessage(ErrorSeverity_t::ErrorSeverity_t_HIGH, "Setup done");
}

void loop()
{
    vTaskDelay(pdMS_TO_TICKS(1000));
}