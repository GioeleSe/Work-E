from flask import Flask, render_template
from flask_socketio import SocketIO
import threading
from enum import Enum
import logging
logger = logging.getLogger(__name__)

class ConfigProperty(Enum):
    SPEED = 0
    FEEDBACK = 1
    DEBUG = 2
    NAVIGATION_TYPE = 3
    ROUTE_POLICY = 4
    RADAR = 5
    SCREEN = 6
    LIGHTS = 7
    HORN = 8
    OBSTACLE_CLEANER = 10
    OBJECT_LOADER = 11
    OBJECT_UNLOADER = 12
    OBJECT_COMPACTER = 13

    @staticmethod
    def from_str(value):
        try:
            return ConfigProperty[value.upper()]
        except KeyError:
            raise ValueError

class DestinationRobot(Enum):
    ERR = -2
    NONE = -1
    ALL = 0
    ROBOT1 = 1
    ROBOT2 = 2
    ROBOT3 = 3
    ROBOT4 = 4

    @classmethod
    def from_int(cls, value: int | str):
        try:
            return cls(int(value))
        except (ValueError, TypeError):
            return cls.ERR

    @classmethod
    def from_str(cls, value: str):
        try:
            return cls[value.upper()]
        except (KeyError, AttributeError):
            return cls.ERR

    def __repr__(self):
        return "<%s.%s>" % (self.__class__.__name__, self._name_)

class Direction(Enum):
    FORWARD = 0
    BACKWARD = 1
    LEFT = 2
    RIGHT = 3
    STOP = 4

    @staticmethod
    def from_str(value):
        try:
            return Direction[value.upper()]
        except KeyError:
            raise ValueError

class HTTPServer:
    def __init__(
        self,
        host="0.0.0.0",
        port=80,
        secret_key="secret!",
        on_control_settings=None,
        on_command=None,
        on_set_property=None,
        on_get_property=None,
        udp_server=None
    ):
        self.host = host
        self.port = port

        self.on_control_settings = on_control_settings or self._default_on_control_settings
        self.on_command = on_command or self._default_on_command
        self.on_set_property = on_set_property or self._default_on_set_property
        self.on_get_property = on_get_property or self._default_on_get_property
        self.udp_server = udp_server

        self.destination_robot = DestinationRobot.NONE

        self.app = Flask(__name__)
        self.app.config["SECRET_KEY"] = secret_key

        self.socketio = SocketIO(
            self.app,
            cors_allowed_origins="*",
            logger=True
        )
        self._register_routes()
        self._register_socketio_handlers()
        self.thread = None

    # -------------------------
    # Flask routes
    # -------------------------
    def _register_routes(self):
        @self.app.route("/", methods=["GET"])
        def default():
            return render_template("index.html")
        
        @self.app.route("/advanced", methods=["GET"])
        def advanced():
            return render_template("advanced_gui.html")

    # -------------------------
    # SocketIO handlers
    # -------------------------
    def _register_socketio_handlers(self):
        @self.socketio.on("control_settings")
        def control_settings(data):
            if self.on_control_settings:
                self.on_control_settings(data)

        @self.socketio.on("command")
        def command(data):
            if self.on_command:
                self.on_command(data)

        @self.socketio.on("set_property")
        def set_property(data):
            if self.on_set_property:
                self.on_set_property(data)

        @self.socketio.on("get_property")
        def get_property(data):
            if self.on_get_property:
                self.on_get_property(data)

    # -------------------------
    # Lifecycle
    # -------------------------
    def start(self, threaded=False):
        """
        threaded=False → blocking (recommended for main process)
        threaded=True  → background thread
        """
        if threaded:
            self.thread = threading.Thread(
                target=self._run,
                daemon=True
            )
            self.thread.start()
        else:
            self._run()

    def _run(self):
        logger.debug(f"[HTTP] Listening on {self.host}:{self.port}")

        self.socketio.run(
            self.app,
            host=self.host,
            port=self.port,
            debug=True,
            use_reloader=False,
            allow_unsafe_werkzeug=True
        )

    def stop(self):
        # Flask-SocketIO does not support clean programmatic shutdown
        # Process-level shutdown is the correct model
        pass

    # -------------------------
    # Outbound events (UDP → GUI)
    # -------------------------
    def emit(self, event, data):
        self.socketio.emit(event, data)

    def link_udp_server(self, udp_server):
        self.udp_server = udp_server

    # incoming packets:
    # {
    #   mode: [manual | auto | settings ]
    #   cmd: [ stop | drive | lights | horn | speed | destination_robot ]
    #   argc: [ forward | reverse | left | right | stop | 0..100 | ON | OFF | PLAY | STOP ]
    # }
    def _default_on_control_settings(self, data):
        logger.debug(f"Changing settings: {data}")
        check = False
        if data.get("mode", 'def') == 'settings':
            if data.get("cmd", 'def') == 'destination_robot':
                if data.get("argc", 'def') in range(-2, 5):
                    # correct robot selection message
                    check =True
        if check:
            self.destination_robot = DestinationRobot.from_int(data.get("argc"))
            logger.warning(f"setting destination_robot to {self.destination_robot}")
            logger.debug(f"Changed destination robot to {self.destination_robot}")
        else:
            logger.debug(f"Invalid control settings message: {data}")

    def _default_on_command(self, data):
        logger.debug(f"Command message:{data}")
        if data.get("mode", 'def') not in ["manual" , "auto"]:
            logger.debug(f"Invalid command mode: {data.get('mode', 'Not defined')}")
        else:
            if data.get("cmd", 'def') == 'stop':
                # plain stop function (stop here -> full stop)
                self.udp_server.send_stop(destination=self.destination_robot)

            elif data.get("cmd", 'def') == 'reset':
                self.udp_server.send_reset(destination=self.destination_robot)

            elif data.get("cmd", 'def') == 'drive':
                if data.get("argc", 'def') in ['forward', 'reverse', 'left', 'right' , 'stop']: # stop here -> motor stop
                    # correct move command
                    direction = Direction.from_str(data.get("argc"))
                    self.udp_server.send_motor_control(destination=self.destination_robot, motor_id=[], direction=direction, angle=0)
                else:
                    logger.debug(f"Invalid drive command: {data}")
            else:
                logger.debug(f"Invalid command: {data}")

    def _default_on_set_property(self, data):
        logger.debug(f"Property set message: {data}")
        check = False
        argc = None
        if data.get("cmd", 'def') in ['speed', 'lights', 'horn']:
            argc = data.get("argc", 'def')
            if argc in range(0,101):
                check = True
        if check:
            prop = ConfigProperty.from_str(data.get("cmd"))
            self.udp_server.send_set_property(destination=self.destination_robot, prop=prop, value=argc)
        else:
            logger.debug(f"Invalid set property message: {data}")

    def _default_on_get_property(self, data):
        logger.debug(f"Property get message: {data}")
        check = False
        if data.get("cmd", 'def') in ['speed', 'lights', 'horn']:
            # correct get property message
            check = True

        if check:
            self.udp_server.send_get_property(destination=self.destination_robot, prop=data.get("cmd"))
        else:
            logger.debug(f"Invalid get property message: {data}")
