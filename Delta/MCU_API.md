# Distributed Robotic Control Protocol (roboControl v1.0)
**UDP Endpoint & Message Structure**
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
All messages use a **logical address** embedded in the payload metadata:
```
robot/{robot_id}/in   → Commands to MCU
robot/{robot_id}/out  → Telemetry from MCU
```

`robot_id` is a unique identifier set for each MCU:

| Robot           | Address            | ID  |
| --------------- | ------------------ | --- |
| Beta            |  `192.168.137.X`   |  1  |
| Charlie         |  `192.168.137.X`   |  2  |
| Delta           |  `192.168.137.X`   |  3  |
| Mario           |  `192.168.137.X`   |  4  |
> **Note:** a static address is not defined, the ID in the endpoint will be used to get a fixed reference (The robot ip will be stored after the first message, sort of ARP).
---

## Operational Flow Summary
### Command Lifecycle
1. Backend sends command → `robot/{id}/in`
2. MCU validates and executes
3. MCU replies with `CommandFeedback` → `robot/{id}/out`
4. MCU continues periodic `Heartbeat`

### Emergency Stop Flow
1. Backend sends `emergency_stop`
2. MCU immediately halts all actuators transitioning to `emergency_stop`
3. MCU still replies with `CommandFeedback` → `robot/{id}/out`
4. MCU continues periodic `Heartbeat`
---

## Implementation Notes
* UDP packet loss is possible; critical commands are retried with if no feedback is received
* MCU has to do the same with important messages such as events and errors
* JSON payload size should remain minimal to avoid fragmentation
---

## Common Message Envelope
All UDP messages have to conform to the following envelope structure.
```json
{
  "protocol": "robot-net/1.0",
  "message_type": "command | feedback | event | error | heartbeat | reset",
  "request_id": "uuid",
  "mode": "manual | auto",
  "payload":{
    // message-specific fields
  },
  "timestamp": 1700000000
}
```

| Field          | Purpose                                            |
| -------------- | -------------------------------------------------- |
| `protocol`     | Versioned protocol identifier (for future updates) |
| `message_type` | Handling classification (for priorities)           |
| `request_id`   | Correlates commands with feedback                  |
| `mode`         | Execution context                                  |
| `payload`      | Message content/parameters                         |
| `timestamp`    | Unix epoch time (for possible msg aging)           |

---

## UDP Endpoints

## Endpoint: `robot/{robot_id}/in`

### Allowed `message_type`

| Value     | Description                                   |
| --------- | --------------------------------------------- |
| `command` | Control instruction to be executed by the MCU |

---

### Supported Commands

The command is identified by `payload.command`.

#### 1. `move`

High-level autonomous navigation command.

| Field             | Type   | Description                                 |
| ----------------- | ------ | ------------------------------------------- |
| `destination.x`   | number | Target X coordinate                         |
| `destination.y`   | number | Target Y coordinate                         |
| `navigation_type` | enum   | `manual`, `checkpoint`, `grid`, `free_move` |
| `route_policy`    | enum   | `shortest`, `safest`, `fast`                |

---

#### 2. `motor_control`

Low-level direct motor control (manual/debug mode).

| Field         | Type    | Allowed Values                         |
| ------------- | ------- | -------------------------------------- |
| `motor_id`    | enum    | `mot1`, `mot2`, `mot3`, `mot4`, `mot5` |
| `direction`   | enum    | `forward`, `backward`, `stop`          |
| `speed`       | number  | Range `0 – 100`                        |
| `duration_ms` | integer | Optional Execution time                |

---

#### 3. `set_config`

Runtime configuration update.

| Field                     | Type  | Allowed Values               |
| ------------------------- | ----- | ---------------------------- |
| `speed_level`             | enum  | `slow`, `normal`, `fast`     |
| `feedback_level`          | enum  | `silent`, `minimal`, `debug` |
| `debug_level`             | enum  | `off`, `basic`, `full`       |
| `enabled_functionalities` | array | Robot-specific modules       |

---

#### 4. `emergency_stop`

Immediate safety command.

| Behavior    | Description                        |
| ----------- | ---------------------------------- |
| Preemption  | Interrupts all ongoing actions     |
| Persistence | Remains active until reset         |
> Generates `CommandFeedback` to confirm stop action

---

#### 4. `reset`

Reset the emergency stop or reset the board to the initial state (according to its current state).

| MCU Context    | Behavior    | Description                               |
| -------------- | ----------- | ----------------------------------------- |
| Em. stop/error | Clear state | Clear error/stop and continue last task   |
| idle/busy      | Reset board | Delete pending request                    |
> Generates `CommandFeedback` in any case
---

### Reliability Rules
* Every command has to include a `request_id`, a sequential 16-bit number to track messages
* MCU has to respond with `CommandFeedback`
* Duplicate commands are ignored
---

## Endpoint: `robot/{robot_id}/out`
### Allowed `message_type`
| Value       | Description                        |
| ----------- | ---------------------------------- |
| `heartbeat` | Periodic status report             |
| `event`     | Asynchronous robot-generated event |
| `feedback`  | Command execution response         |
| `error`     | Fault or unrecoverable issue       |

---

### Message Definitions
#### 1. `Heartbeat`
Periodic health and presence signal.
| Field         | Type    | Description           |
| ------------- | ------- | --------------------- |
| `state`       | enum    | See Robot States      |
| `rssi`        | integer | Signal strength (dBm) |

Transmission:
* sent at a fixed interval
* Used for robot discovery and liveness
---

#### 2. `Feedback`
Response to a previously issued command.
| Field           | Type   | Allowed Values                  |
| --------------- | ------ | ------------------------------- |
| `status`        | enum   | `success`, `failure`, `pending` |
| `error.code`    | string | Optional                        |
| `error.message` | string | Optional                        |

Rules:
* `request_id` has to match the originating command
* Sent exactly once per command (but n debug messages can be sent)
---

#### 3. `Event`
Asynchronous runtime event.
| Event               | Description                  |
| ------------------- | ---------------------------- |
| `obstacle_detected` | Obstacle encountered         |
| `obstacle_removed`  | Obstacle cleared             |
| `poi_reached`       | Point of interest reached    |
| `load_collected`    | Load successfully collected  |
| `load_disposed`     | Load successfully disposed   |
| `reroute_required`  | New route computation needed |

`details` may include robot-specific metadata.

---

#### 4. `Error`
Both unrecoverable and recoverable failure.

| Field           | Type   | Allowed Values                  |
| --------------- | ------ | ------------------------------- |
| `severity`      | enum   | `low`, `mid`, `high`            |
| `error.code`    | string | Optional                        |
| `error.message` | string | Optional                        |