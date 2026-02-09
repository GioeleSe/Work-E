# Distributed Robotic Control Protocol (roboControl v1.0)
## Overview
This document defines the UDP communication endpoints and message semantics for the **Robotic Control Protocol**.
* Transport: **UDP**
* Encoding: **JSON**
* Envelope: **Common header + message-specific payload**
* Modes:
  * **Manual / Debug** – direct motor-level control
  * **Automatic** – task- and navigation-oriented control

The protocol is asymmetric:
* **Backend → MCU**: Commands
* **MCU → Backend**: Telemetry, events, and feedback
---

## Network Topology
| Role     | Direction | Address Pattern  | Description                  |
| -------- | --------- | ---------------- | ---------------------------- |
| Backend  | Listens   | `0.0.0.0:8000`   | Receives feedback            |
| Backend  | Sends     | `192.168.137.X`  | Send commands                |
| MCU Node | Listens   | `0.0.0.0:8000`   | Receives commands            |
| MCU Node | Sends     | `192.168.137.1`  | Heartbeats, events, feedback |

> **Note:** UDP ports are pre-defined and must be consistent across all nodes.
<br>
>   The address of the server is defined as the gateway of the hotspot network.
---

## Addressing Model

> A static address is not defined, the ID in the envelope will be used to get a fixed reference (The robot ip will be stored after the first message, sort of ARP).

| Robot           | Address            | ID  |
| --------------- | ------------------ | --- |
| Beta            |  `192.168.137.X`   |  1  |
| Charlie         |  `192.168.137.X`   |  2  |
| Delta           |  `192.168.137.X`   |  3  |
| Mario           |  `192.168.137.X`   |  4  |
---

## Operational Flow Summary
### Command Lifecycle
1. Backend sends command → `{robot ip}:8000`
2. MCU validates and executes
3. MCU replies with `CommandFeedback` → `{server ip}:8000`
> MCU continues periodic `Heartbeat`
> MCU sends asynchronous debug messages `Debug`

---

## Implementation Notes
* UDP packet loss is possible; critical commands are retried with if no feedback is received
* MCU has to do the same with important messages such as events and errors
---

## Common Message Envelope
All UDP messages have to conform to the following envelope structure.
```json
{
  "protocol": "robot-net/1.0",
  "robot_id": [1 | 2 | 3 | 4],
  "message_type": "[command | feedback | event | debug | error | heartbeat ]",
  "request_id": "uuid", // 16 random bits defined by the server
  "mode": "manual | auto",
  "payload":{
    // message-specific fields
  },
  "timestamp": 1770483512000 // Timestamp given by first getting the Epoch time (fetch it from ntp server after wifi connection, a uint32_t should be sufficient)
}
```

| Field          | Purpose                                            |
| -------------- | -------------------------------------------------- |
| `protocol`     | Versioned protocol identifier (for future updates) |
| `robot_id`     | Logic-Ip connection of the robot                   |
| `message_type` | Handling classification (for priorities)           |
| `request_id`   | Correlates commands with feedback                  |
| `mode`         | Execution context                                  |
| `payload`      | Message content/parameters                         |
| `timestamp`    | Unix epoch time (for msg aging)                    |

---

## UDP incoming (to each robot) messages:

### Allowed `message_type`

| Value     | Description                                   |
| --------- | --------------------------------------------- |
| `command` | Control instruction to be executed by the MCU |

---

### Supported Commands

The command is identified by `payload.command`.

#### 1. `move`

High-level autonomous navigation command.

| Field                    | Type   | Description                                 |
| ------------------------ | ------ | ------------------------------------------- |
| `destination_x`          | int    | Target X coordinate                         |
| `destination_y`          | int    | Target Y coordinate                         |
| `destination_checkpoint` | enum   | `home`,`load`,`base`,                       |
| `navigation_type`        | enum   | `manual`, `checkpoint`, `grid`, `free_move` |
| `route_policy`           | enum   | `shortest`, `safest`, `fast`                |

Note:  (destination_checkpoint) base is the
      home is the starting point

C example:
```C
  // Server-Common Structures
  typedef enum
  {
    HOME = 0, // starting point
    LOAD = 1, // position of the load to collect 
    BASE = 2  // load destination
  } DestinationCheckpoint;
  typedef enum {
    MANUAL = 0, // manual GUI, robot as dummy collection of motors
    CHECKPOINT = 1, //home-load-target navigation
    GRID = 2, // prefixed grid pattern (90° rotations only at turning points, like a chess board)
    FREE_MOVE = 3 // no target, no grid, just an happy robot going around (for future extensions obv)
  } NavigationType;
  typedef enum {
    SHORTEST = 0,
    SAFEST = 1,
    FAST = 2
  } RoutePolicy;
  // Payload of the incoming message:
  typedef struct MovePayload{
      int destination_x;  // 2d coordinates for grid movements, the robot will follow a grid pattern to the defined destination
      int destination_y;  // ...
      DestinationCheckpoint destination_checkpoint;
      NavigationType navigation_type;
      RoutePolicy route_policy;
  } MovePayload;
```
---

#### 2. `motor_control`

Low-level direct motor control (manual/debug mode).

| Field         | Type          | Allowed Values                                        |
| ------------- | ------------- | ----------------------------------------------------- |
| `motor_id`    | list(enum)    | {`mot1`, `mot2`, `mot3`, `mot4`, `mot5`, `end_mot`}   |
| `direction`   | enum          | `FORWARD`, `BACKWARD`,`LEFT`,`RIGHT`, `STOP`          |
| `speed`       | number        | Range `0 – 100`                                       |
| `angle`       | number        | Range `-360 – +360`                                   |
| `duration_ms` | integer       | Optional Execution time                               |

Note: can activate multiple motors at the same time, the end of the list will be the end_mot value.
If the list is empty the intended action is to move the robot as a normal car.
The directions left, right are meant to be rotations only (on spot rotation)

C example:
```C
  // Server-Common Structures
  typedef enum{
      END_MOT = -1, // end of motor list
      RES = 0,    // reserved for now
      MOT1 = 1,   // left car movement motor
      MOT2 = 2,   // right car movement motor
      MOT3 = 3,   // additional motors
      MOT4 = 4,   // additional motors
      MOT5 = 5,   // ...
      MOT6 = 6    // ...
  } Motors;
  typedef enum{
    FORWARD = 0,
    BACKWARD = 1,
    LEFT = 2,
    RIGHT = 3,
    STOP = 4 // only for motors, it's a simple way to stop them NOT the emergency one
  } Direction;
  // Payload of the incoming message:
  typedef struct MotorControlPayload{
    Motors* motor_ids;     // can activate multiple motors at once, if the vector is empty the intended action is to drive it as a car
    Direction direction;    // can be LEFT or RIGHT only if motor_ids is empty, ignored otherwise
    uint8_t speed;      // 0-100, basically the direct pwm ratio
    int angle;          // from -360 to +360, on spot rotation angle
    uint32_t duration;  // can be 0 or greater (0 -> keep running, otherwise set a timeout)
  } MotorControlPayload;
```

---

#### 3. `set_config`

Runtime configuration update.

| Field                     | Type  | Allowed Values               |
| ------------------------- | ----- | ---------------------------- |
| `prop`                    | str   | `speed_level`,`feedback_level`,`debug_level`,`obstacle_cleaner`, `object_unloader`, `object_compacter`, `object_loader` |
| `new_value`               | str   | see list below |

Runtime possible properties updates.

| Property                  | Type      | Allowed Values               |
| ------------------------- | --------- | ---------------------------- |
| `SPEED`                   | enum      | `slow`, `normal`, `fast`     |
| `FEEDBACK`                | enum      | `silent`, `minimal`, `debug` |
| `DEBUG`                   | enum      | `off`, `basic`, `full`       |
| `NAVIGATION_TYPE`         | enum      | `manual`, `checkpoint`, `grid`, `free_move` |
| `ROUTE_POLICY`            | enum      | `shortest`, `safest`, `fast`                |
| `RADAR `                  | bit-bool  |  1 (`on`), 0(`off`)          |
| `SCREEN `                 | bit-bool  |  1 (`on`), 0(`off`)          |
| `OBSTACLE_CLEANER `       | bit-bool  |  ...                         |
| `OBJECT_LOADER `          | bit-bool  |  ...                         |
| `OBJECT_UNLOADER `        | bit-bool  |  ...                         |
| `OBJECT_COMPACTER `       | bit-bool  |  1 (`on`), 0(`off`)          |



C example:
```C
  // Server-Common Structures
  typedef enum{
    SPEED = 0,
    FEEDBACK = 1,
    DEBUG = 2,
    NAVIGATION_TYPE = 3,
    ROUTE_POLICY = 4,
    RADAR = 5,
    SCREEN = 6,
    OBSTACLE_CLEANER = 10,  // range gap for future updates
    OBJECT_LOADER = 11,
    OBJECT_UNLOADER = 12,
    OBJECT_COMPACTER = 13
  }ConfigFields;
  typedef struct SetConfigPayload
  {
    ConfigFields prop; // single prop from ConfigFields list
    void *new_value;  // new value type depends on what's the property
  } SetConfigPayload;
```

---

#### 4. `get_config`

Get current configuration.

| Field                     | Type  | Allowed Values               |
| ------------------------- | ----- | ---------------------------- |
| `prop`                    | str   | `speed_level`,`feedback_level`,`debug_level`,`obstacle_cleaner`, `object_unloader`, `object_compacter`, `object_loader` |

The expected value (server side) is from the table above (in set_config description)

C example:
```C
  // Server-Common Structures
  typedef enum{
    SPEED = 0,
    FEEDBACK = 1,
    DEBUG = 2,
    NAVIGATION_TYPE = 3,
    ROUTE_POLICY = 4,
    RADAR = 5,
    SCREEN = 6,
    OBSTACLE_CLEANER = 10,  // range gap for future updates
    OBJECT_LOADER = 11,
    OBJECT_UNLOADER = 12,
    OBJECT_COMPACTER = 13
  }ConfigFields;
  typedef struct GetConfigPayload
  {
    ConfigFields prop; // expected one of the ConfigFields to return its value
  } GetConfigPayload;
```

---
#### 5. `emergency_stop`

Immediate safety command.

| Field                     | Type  | Allowed Values               |
| ------------------------- | ----- | ---------------------------- |
| `stop`                    | str   | `stop`                       |

| Behavior    | Description                        |
| ----------- | ---------------------------------- |
| Preemption  | Interrupts all ongoing actions     |
| Persistence | Remains active until reset         |
> Generates `CommandFeedback` to confirm stop action

C example:
```C
  typedef struct EmergencyStopPayload
  {
    const char *stop; // gotta simply match the "stop" keyword to stop the tasks currently running
  } EmergencyStopPayload;
```
---

#### 6. `reset`

Reset the emergency stop or reset the board to the initial state (according to its current state).

| Field                     | Type  | Allowed Values               |
| ------------------------- | ----- | ---------------------------- |
| `reset`                   | str   | `reset`                      |

| MCU Context    | Behavior    | Description                               |
| -------------- | ----------- | ----------------------------------------- |
| Em. stop/error | Clear state | Clear error/stop and continue last task   |
| idle/busy      | Reset board | Delete pending request                    |
> Generates `CommandFeedback` in any case


C example:
```C
  typedef struct ResetPayload
  {
    const char *reset; // gotta simply match the "reset" keyword to reset the board (idle/busy) or clear the error state (or clear the active emergency stop state)
  } ResetPayload;
```

---

## UDP outgoing (from each robot) messages:

### Allowed `message_type`
| Value       | Description                                         |
| ----------- | --------------------------------------------------- |
| `heartbeat` | Periodic status report                              |
| `event`     | Asynchronous robot-generated event                  |
| `feedback`  | Command execution response                          |
| `debug`     | Non relevant events and internal functionalities    |
| `error`     | Fault or unrecoverable issue                        |

---

### Message Definitions
#### 1. `Heartbeat`
Periodic health and presence signal.
| Field         | Type    | Description              |
| ------------- | ------- | ------------------------ |
| `state`       | enum    | `idle`, `busy`, `error`  |
| `rssi`        | integer | Signal strength (dBm)    |

Transmission:
* sent at a fixed interval (2 seconds)
* used for robot discovery and liveness
* expected every 2 seconds, after 6 seconds (3 missing packets) the board is considered disconnected


C example:
```C
typedef struct{
    IDLE = 0,
    BUSY = 1,
    ERR = 2
}RobotState;

// function example, missing things like debug and controls! 
DynamicJsonDocument dummyHeartbeat() {
  DynamicJsonDocument doc(2048);
  doc["state"] = (int)currentRobotState;  // send only the raw int
  doc["rssi"] = WiFi.RSSI();  // direct rssi (no error checking, BAD as RSSI() can return error!)
  return doc; // still need to include the envelope fields, don't send directly this.
}
```
---

#### 2. `Feedback`
Response to a previously issued command.
| Field           | Type   | Allowed Values                               |
| --------------- | ------ | -------------------------------------------- |
| `status`        | enum   | `success`, `failure`, `pending`              |
| `error_code`    | int    | Optional (checked only if status = failure)  |
| `error_message` | string | Optional ( ... )                             |

C example:
```C
  // Server-Common Structures
  typedef struct{
    SUCCESS = 0,
    FAILURE = 1,
    PENDING = 2
  }ActionResult;
  typedef int ErrorCode_t;

  typedef struct FeedbackMsg_t{
    ActionResult status;
    ErrorCode_t error_code;
    char* error_message;
  }FeedbackMsg_t;
```
The total structure sent as serialized JSON will be something like:
```Json
{
  "protocol": "robot-net/1.0",
  "message_type": "feedback",
  "request_id": "77C4",   // random 16 bits in HEX from previous command message
  "mode": 0,  // manual mode ((int)MessageMode.Manual == 0)
  "payload":{
    "status": 0,        // idle (defined IDLE=0 in RobotState enum)
    "error_code": 0,    // not an error -> errno 0 (error number = 0) 
    "error_message": "/0", // empty string, use debug to send low priority messages
  },
  "timestamp": 1770482925000
}
```

---

#### 3. `Event`
Asynchronous runtime event.

| Field           | Type   | Allowed Values                               |
| --------------- | ------ | -------------------------------------------- |
| `event`         | enum   | see below (long list)                        |
| `details`       | string | Optional description of the event            |

| Event               | Description                  |
| ------------------- | ---------------------------- |
| `obstacle_detected` | Obstacle encountered         |
| `obstacle_removed`  | Obstacle cleared             |
| `poi_reached`       | Point of interest reached    |
| `load_collected`    | Load successfully collected  |
| `load_disposed`     | Load successfully disposed   |
| `reroute_required`  | New route computation needed |
| `missing_load`      | Load not found               |


C example:
```C
  // Server-Common Structures
  typedef enum{
    obstacle_detected = 10,
    obstacle_removed =11,
    poi_reached = 12,
    load_collected = 13,
    load_disposed = 14,
    reroute_required = 15,
    missing_load = 16
  }FeedbackEvent_t;

  typedef struct EventMsg_t{
    FeedbackEvent_t event;
    const char* details; 
  } EventMsg_t;
```
---

#### 4. `Debug`
All possible messages of the internal state like low level error or warnings or simple informations
like command execution steps.
These messages will not be checked from the code but only printed in the UI.

| Field           | Type   | Allowed Values                  |
| --------------- | ------ | ------------------------------- |
| `message`       | string | Even smiles if you want! @.@    |


C example:
```C
  typedef struct DebugMsg_t{
    const char* message;    // the only field for now, could be extended!
  }DebugMsg_t

```

---

#### 5. `Error`
Both unrecoverable and recoverable failure.

| Field           | Type   | Allowed Values                  |
| --------------- | ------ | ------------------------------- |
| `severity`      | enum   | `low`, `mid`, `high`            |
| `error_code`    | int    | code mapped to specific error   |



C example:
```C
  // Server-Common Structures
  typedef enum{
    LOW = 0,
    MID = 1,
    HIGH = 2
  } ErrorSeverity_t;

  typedef struct ErrorMsg_t{
    ErrorSeverity_t severity;
    int error_code;
  }ErrorMsg_t

```
