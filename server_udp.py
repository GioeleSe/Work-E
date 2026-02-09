from asyncio import SendfileNotAvailableError
import datetime
import random
import socket
import json
import time
import uuid
import threading
import logging
from datetime import timezone
from enum import Enum
from pydantic.v1 import UUID4
from server_http import Direction, DestinationRobot, ConfigProperty

logger = logging.getLogger(__name__)


class MessageMode(Enum):
    MANUAL = 0
    AUTO = 1


class CommandType(Enum):
    GET_PROPERTY = 0
    SET_PROPERTY = 1
    MOTOR_CONTROL = 2
    MOVE = 3
    EMERGENCY_STOP = 4
    RESET = 5


class MessageType(Enum):
    COMMAND = 0
    FEEDBACK = 1
    EVENT = 2
    ERROR = 3
    HEARTBEAT = 4
    RESET = 5


class DestinationCheckpoint(Enum):
    HOME = 0
    LOAD = 1
    BASE = 2


class NavigationType(Enum):
    MANUAL = 0
    CHECKPOINT = 1
    GRID = 2
    FREE_MOVE = 3


class RoutePolicy(Enum):
    SHORTEST = 0
    SAFEST = 1
    FAST = 2


class SpeedLevel(Enum):
    SLOW = 0
    NORMAL = 1
    FAST = 2


class DebugLevel(Enum):
    OFF = 0
    BASIC = 1
    FULL = 2


class FeedbackLevel(Enum):
    SILENT = 0
    MINIMAL = 1
    DEBUG = 2


class Motors(Enum):
    END_MOT = -1
    RES = 0
    MOT1 = 1
    MOT2 = 2
    MOT3 = 3
    MOT4 = 4
    MOT5 = 5
    MOT6 = 6


class FeedbackEvent(Enum):
    obstacle_detected = 10
    obstacle_removed = 11
    poi_reached = 12
    load_collected = 13
    load_disposed = 14
    reroute_required = 15
    missing_load = 16


class ErrorSeverity(Enum):
    LOW = 0
    MID = 1
    HIGH = 2


class UDPMessage:
    def __init__(self, protocol: str = "robot-net/1.0", message_type: MessageType = MessageType.COMMAND,
                 request_uuid: UUID4 = None, mode: MessageMode = MessageMode.MANUAL, payload=None):
        request_uuid = uuid.uuid4().bytes[0:1] if not request_uuid else request_uuid.bytes[0:1]

        self.protocol = protocol
        self.message_type = message_type
        self.request_id = request_uuid
        self.mode = mode
        self.payload = payload
        self.timestamp = datetime.datetime.now(timezone.utc)

    def to_dict(self) -> dict:
        return {
            "protocol": self.protocol,
            "message_type": _serialize_value(self.message_type),
            "request_id": self.request_id.hex(),
            "mode": _serialize_value(self.mode),
            "timestamp": self.timestamp.isoformat(),
            "payload": _serialize_value(self.payload),
        }

    def to_json(self) -> str:
        return json.dumps(self.to_dict(), separators=(",", ":"))

    @staticmethod
    def reset_board(request_uuid: UUID4 = None):
        return UDPMessage(
            request_uuid=request_uuid,
            payload={"command": CommandType.RESET, "reset": "reset"},
        )

    @staticmethod
    def emergency_stop(request_uuid: UUID4 = None):
        return UDPMessage(
            request_uuid=request_uuid,
            payload={"command": CommandType.EMERGENCY_STOP, "stop": "stop"},
        )

    @staticmethod
    def set_property(request_uuid: UUID4 = None, prop: ConfigProperty = None, new_value=None):
        if prop is None or new_value is None:
            raise ValueError
        message = UDPMessage(request_uuid=request_uuid)
        message.payload = {
            "command": CommandType.SET_PROPERTY,
            "prop": prop,
            "new_value": new_value
        }
        return message

    @staticmethod
    def get_property(request_uuid: UUID4 = None, prop: ConfigProperty = None):
        return UDPMessage(
            request_uuid=request_uuid,
            payload={
                "command": CommandType.GET_PROPERTY,
                "prop": prop,
            },
        )

    # continuous control = duration_ms set to 0
    # motor_id can be a list of multiple motors to drive
    @staticmethod
    def motor_control(request_uuid: UUID4 = None, motor_list: list = None, direction: Direction = Direction.STOP,
                      angle: int = 0, speed: int = 100, duration_ms: int = 0):
        if direction is None:
            raise ValueError
        if motor_list is None:
            motor_list = [Motors.MOT1, Motors.MOT2, Motors.END_MOT]
        else:
            motor_list.append(Motors.END_MOT)

        message = UDPMessage(request_uuid=request_uuid)
        message.payload = {
            "command": CommandType.MOTOR_CONTROL,
            "motor_id": motor_list,
            "direction": direction,
            "speed": speed,
            "angle": angle,
            "duration_ms": duration_ms
        }
        return message

    @staticmethod
    def move(request_uuid: UUID4 = None, move_destination=None, navigation_type: NavigationType = NavigationType.MANUAL,
             route_policy: RoutePolicy = RoutePolicy.SHORTEST):
        message = UDPMessage(request_uuid=request_uuid)
        message.payload = {
            "command": CommandType.MOVE,
            "destination_x": move_destination.x,
            "destination_y": move_destination.y,
            "destination_checkpoint": move_destination.checkpoint,
            "navigation_type": navigation_type,
            "route_policy": route_policy
        }
        return message


def _serialize_value(value):
    if isinstance(value, Enum):
        return value.value  # send num of enums instead of labels
    if isinstance(value, (bytes, bytearray)):
        return value.hex()
    if isinstance(value, datetime.datetime):
        # Format: 2026-02-09T14:30:05.123456Z
        return value.isoformat(timespec='microseconds').replace('+00:00', 'Z')
    if isinstance(value, list):
        return [_serialize_value(v) for v in value]
    if isinstance(value, dict):
        return {k: _serialize_value(v) for k, v in value.items()}
    return value


class UDPServer:
    def __init__(
            self,
            bind_ip,
            bind_port,
            protocol_id,
            frame_size=1024,
            on_message=None,
            http_server=None,
            localhost_dev=False,
    ):
        self.bind_ip = bind_ip
        self.bind_port = bind_port
        self.protocol_id = protocol_id
        self.frame_size = frame_size

        self.on_message = on_message
        self.http_server = http_server
        self.localhost_dev = localhost_dev

        self.robot_prop_table = {
            1: {
                "name": "Beta",
                "motor_speed": 100,
            },
            2: {
                "name": "Charlie",
                "motor_speed": 100,
            },
            3: {
                "name": "Delta",
                "motor_speed": 100,
            },
            4: {
                "name": "Mario",
                "motor_speed": 100,
            },
        }
        self.robot_ip_table = {
            1: '192.168.1.100',
            2: '192.168.1.101',
            3: '192.168.1.102',
            4: '192.168.1.103',
        }
        self.robot_last_seen_table = {}

        self.sock = None
        self.running = False
        self.thread = None

    # -------------------------
    # Lifecycle
    # -------------------------
    def start(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind((self.bind_ip, self.bind_port))
        self.sock.settimeout(1.0)

        logger.info(
            "[UDP] Listening on %s:%s (protocol=%s, frame_size=%d)",
            self.bind_ip,
            self.bind_port,
            self.protocol_id,
            self.frame_size,
        )

        self.running = True
        self.thread = threading.Thread(
            target=self._run_loop,
            daemon=True
        )
        self.thread.start()

        logger.debug(f"Listening on {self.bind_ip}:{self.bind_port}")

    def stop(self):
        self.running = False
        if self.sock:
            self.sock.close()
        if self.thread:
            self.thread.join(timeout=2)

        logger.debug("Stopped")

    # -------------------------
    # Receive loop
    # -------------------------
    def _run_loop(self):
        while self.running:
            try:
                raw, addr = self.sock.recvfrom(self.frame_size)
            except socket.timeout:
                continue
            except OSError:
                break

            sender_ip, _ = addr

            try:
                msg = json.loads(raw.decode())
                logger.debug(
                    "[UDP RX] from %s robot_id=%s message_type=%s payload=%s",
                    sender_ip,
                    robot_id,
                    msg.get("message_type"),
                    msg.get("payload"),
                )

            except Exception:
                continue

            # ------------------
            # Incoming messages parsing (according to the protocol structure)
            # ------------------
            protocol_id = msg.get("protocol", None)
            robot_id = msg.get("robot_id", None)

            # set up a complete parsing here
            if protocol_id != self.protocol_id:
                continue
            if robot_id not in range(1, 5):
                continue

            robot_stored_ip = self.robot_ip_table.get(robot_id, 'New')
            if robot_stored_ip == 'New':
                self.robot_ip_table[robot_id] = sender_ip
                logger.info(f"New robot id-ip discovered: ID{robot_id}-IP{sender_ip}")
            elif robot_stored_ip != sender_ip:
                self.robot_ip_table[robot_id] = sender_ip
                self.robot_last_seen_table[robot_id] = time.time()
                logger.info(f"New id-ip mapping: ID{robot_id}-IP{sender_ip}")

            message_type = msg.get("message_type")

            self.message_handler(robot_id, message_type, msg)

    def link_http_server(self, http_server):
        self.http_server = http_server

    # --------------------------
    # Incoming messages handlers
    # --------------------------
    def heartbeat_handler(self, robot_id, message_type, msg):
        payload = msg.get("payload", {})
        timestamp = msg.get("timestamp")
        self.http_server.emit(
            "robot_heartbeat",
            {
                "robot_id": robot_id,
                "state": payload.get("state"),
                "rssi": payload.get("rssi"),
                "timestamp": timestamp
            }
        )

    def event_handler(self, robot_id, message_type, msg):
        payload = msg.get("payload", {})
        self.http_server.emit(
            "robot_event",
            {
                "robot_id": robot_id,
                **payload
            }
        )

    def feedback_handler(self, robot_id, message_type, msg):
        request_id = msg.get("request_id")
        payload = msg.get("payload", {})
        self.http_server.emit(
            "robot_feedback",
            {
                "robot_id": robot_id,
                "request_id": request_id,
                "status": payload.get("status"),
                "error": payload.get("error")
            }
        )

    def error_handler(self, robot_id, message_type, msg):
        payload = msg.get("payload", {})
        self.http_server.emit(
            "robot_error",
            {
                "robot_id": robot_id,
                "severity": payload.get("severity"),
                "error": payload.get("error")
            }
        )

    def message_handler(self, robot_id, message_type, msg):
        if message_type == "heartbeat":
            self.heartbeat_handler(robot_id, message_type, msg)

        elif message_type == "event":
            self.event_handler(robot_id, message_type, msg)

        elif message_type == "feedback":
            self.feedback_handler(robot_id, message_type, msg)

        elif message_type == "error":
            self.error_handler(robot_id, message_type, msg)

        else:
            # Unknown or unsupported message type
            logger.debug(f"robot_unknown_message: {msg}")

    # -------------------------
    # Outgoing command
    # -------------------------
    def send_reset(self, destination: DestinationRobot = DestinationRobot.ALL):
        # build reset message according to protocol
        message = UDPMessage.reset_board()
        logger.debug(f"board reset message: {message.to_json()}")
        self.message_sender(robot_id=destination, message=message)

    def send_stop(self, destination: DestinationRobot = DestinationRobot.ALL):
        # build stop message according to protocol
        message = UDPMessage.emergency_stop()
        logger.debug(f"emergency stop message: {message.to_json()}")
        self.message_sender(robot_id=destination, message=message)

    def send_move(self, destination: DestinationRobot = DestinationRobot.NONE, move_destination=None,
                  route_policy: RoutePolicy = RoutePolicy.SHORTEST,
                  navigation_type: NavigationType = NavigationType.MANUAL):
        # build drive message according to protocol
        message = UDPMessage.move(move_destination=move_destination, route_policy=route_policy,
                                  navigation_type=navigation_type)
        logger.debug(f"drive message: {message.to_json()}")
        self.message_sender(robot_id=destination, message=message)

    def send_motor_control(self, destination: DestinationRobot = DestinationRobot.NONE, motor_id: list = None,
                           direction: Direction = Direction.STOP, angle: int = 0, duration_ms: int = 0):
        speed = 100
        if destination not in [DestinationRobot.ERR, DestinationRobot.NONE, DestinationRobot.ALL]:
            speed = self.robot_prop_table[destination.value].get("motor_speed", 100)
        message = UDPMessage.motor_control(motor_list=motor_id, direction=direction, speed=speed, angle=angle,
                                           duration_ms=duration_ms)
        logger.debug(f"motor control message: {message.to_json()}")
        self.message_sender(robot_id=destination, message=message)

    def send_set_property(self, destination: DestinationRobot = DestinationRobot.NONE, prop: ConfigProperty = None,
                          value=None):
        # build set property message according to protocol
        message = UDPMessage.set_property(prop=prop, new_value=value)
        logger.debug(f"set property message: {message.to_json()}")
        self.message_sender(robot_id=destination, message=message)

    def send_get_property(self, destination: DestinationRobot = DestinationRobot.NONE, prop: ConfigProperty = None):
        # build get property message according to protocol
        message = UDPMessage.get_property(prop=prop)
        logger.debug(f"get property message: {message.to_json()}")
        self.message_sender(robot_id=destination, message=message)

    def message_sender(self, robot_id: DestinationRobot = DestinationRobot.ROBOT1, message: UDPMessage = None):
        if robot_id == DestinationRobot.ALL:
            for this_id, ip in self.robot_ip_table.items():
                # send same message to each known ip address
                if not ip:
                    logger.debug(f"No IP known for robot {this_id}")
                    continue
                self.sock.sendto(
                    json.dumps(message).encode(),
                    (ip, self.bind_port)
                )

        elif robot_id == DestinationRobot.NONE:
            # debug print of the message
            logger.debug(
                f"Destination of the message was set to NONE. Debug printing instead of socket sendto:\n{message.to_json()}")
        elif robot_id == DestinationRobot.ERR:
            logger.warning(
                f"Destination of the message was set to ERR. Debug printing instead of socket sendto:\n{message.to_json()}")
        else:
            ip = self.robot_ip_table.get(robot_id.value)
            if not ip:
                logger.debug(f"No IP known for robot {robot_id}")
            else:
                encoded_msg = message.to_json().encode()
                self.sock.sendto(
                    encoded_msg,
                    (ip, self.bind_port)
                )
