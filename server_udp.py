import socket
import json
import time
import uuid
import threading
import logging
from server_http import DestinationRobot

logger = logging.getLogger(__name__)
logging.basicConfig(filename=f'server.log', encoding='utf-8', level=logging.DEBUG)
logging.getLogger("werkzeug").setLevel(logging.WARNING)
logging.getLogger("socketio.server").setLevel(logging.WARNING)

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

        self.robot_ip_table = {}
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
            if robot_id not in range(1,5):
                continue

            robot_stored_ip = self.robot_ip_table.get(robot_id, 'New')
            if robot_stored_ip == 'New':
                self.robot_ip_table[robot_id] = sender_ip
                print(f"New robot id-ip discovered: ID{robot_id}-IP{sender_ip}")
            elif robot_stored_ip != sender_ip:
                self.robot_ip_table[robot_id] = sender_ip
                self.robot_last_seen_table[robot_id] = time.time()
                print

            message_type = msg.get("message_type")

            if self.on_message:
                self.on_message(robot_id, message_type, msg)

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
    def send_stop(self, destination:DestinationRobot=DestinationRobot.ALL):
        # build stop message according to protocol
        pass
    def send_drive(self, destination:DestinationRobot=DestinationRobot.NONE, direction:str='forward'):
        # build drive message according to protocol
        pass
    def send_set_property(self, destination:DestinationRobot=DestinationRobot.NONE, property:str=None, value=None):
        # build set property message according to protocol
        pass
    def send_get_property(self, destination:DestinationRobot=DestinationRobot.NONE, property:str=None):
        # build set property message according to protocol
        pass

    def send_command(self, robot_id, payload, mode="manual"):
        ip = self.robot_ip_table.get(robot_id)
        if not ip:
            logger.debug(f"No IP known for robot {robot_id}")
            if self.localhost_dev:
                logger.debug("Using localhost for localhost_dev")
                ip = "127.0.0.1"

        msg = {
            "protocol": self.protocol_id,
            "robot_id": robot_id,
            "message_type": "command",
            "request_id": str(uuid.uuid4()),
            "mode": mode,
            "payload": payload,
            "timestamp": int(time.time())
        }

        self.sock.sendto(
            json.dumps(msg).encode(),
            (ip, self.bind_port)
        )
