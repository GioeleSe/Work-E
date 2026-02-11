#pragma once

// Auto-extracted from test/jsons.txt (comments removed).
// Keep in sync with jsons.txt when updating sample messages.

static const char* kJsons[] = {
  R"JSON({"protocol":"robot-net/1.0","message_type":0,"request_id":"8f","mode":0,"timestamp":"2026-02-09T14:42:58.584303+00:00","payload":{"command":5,"reset":"reset"}})JSON",
  R"JSON({"protocol":"robot-net/1.0","message_type":0,"request_id":"91","mode":0,"timestamp":"2026-02-09T14:28:33.468733+00:00","payload":{"command":2,"motor_id":[-1],"direction":0,"speed":100,"angle":0,"duration_ms":0}})JSON",
  R"JSON({"protocol":"robot-net/1.0","message_type":0,"request_id":"89","mode":0,"timestamp":"2026-02-09T14:30:33.468733+00:00","payload":{"command":2,"motor_id":[-1],"direction":2,"speed":100,"angle":0,"duration_ms":0}})JSON",
  R"JSON({"protocol":"robot-net/1.0","message_type":0,"request_id":"ed","mode":0,"timestamp":"2026-02-09T14:32:33.468733+00:00","payload":{"command":2,"motor_id":[-1],"direction":4,"speed":100,"angle":0,"duration_ms":0}})JSON",
  R"JSON({"protocol":"robot-net/1.0","message_type":0,"request_id":"91","mode":0,"timestamp":"2026-02-09T14:32:48.191140+00:00","payload":{"command":1,"prop":0,"new_value":77}})JSON",
  R"JSON({"protocol":"robot-net/1.0","message_type":0,"request_id":"b8","mode":0,"timestamp":"2026-02-09T14:33:58.973698+00:00","payload":{"command":1,"prop":6,"new_value":0}})JSON"
};

static const size_t kJsonCount = sizeof(kJsons) / sizeof(kJsons[0]);
