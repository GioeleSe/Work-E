from flask import Flask, render_template
from flask_socketio import SocketIO
import threading
from enum import Enum
import logging
logger = logging.getLogger(__name__)
logging.basicConfig(filename=f'server.log', encoding='utf-8', level=logging.DEBUG)

class DestinationRobot(Enum):
    ERR = -2,
    NONE = -1,
    ALL = 0,
    ROBOT1 = 1,
    ROBOT2 = 2,
    ROBOT3 = 3,
    ROBOT4 = 4


    @staticmethod
    def from_int(value: int | str):
        try:
            value = int(value)
        except:
            return DestinationRobot.ERR

        if value == -2:
            return DestinationRobot.ERR
        elif value == -1:
            return DestinationRobot.NONE
        elif value == 0:
            return DestinationRobot.ALL
        elif value == 1:
            return DestinationRobot.ROBOT1
        elif value == 2:
            return DestinationRobot.ROBOT2
        elif value == 3:
            return DestinationRobot.ROBOT3
        elif value == 4:
            return DestinationRobot.ROBOT4
        else:
            return DestinationRobot.ERR

    @staticmethod
    def from_str(value: str):
        if value == "ERR":
            return DestinationRobot.ERR
        elif value == "NONE":
            return DestinationRobot.NONE
        elif value == "ALL":
            return DestinationRobot.ALL
        elif value == "ROBOT1":
            return DestinationRobot.ROBOT1
        elif value == "ROBOT2":
            return DestinationRobot.ROBOT2
        elif value == "ROBOT3":
            return DestinationRobot.ROBOT3
        elif value == "ROBOT4":
            return DestinationRobot.ROBOT4
        else:
            return DestinationRobot.ERR

    def __repr__(self):
        return "<%s.%s>" % (self.__class__.__name__, self._name_)




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
        def root():
            return render_template("index.html")

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
            logger.debug(f"Changed destination robot to {self.destination_robot}")
        else:
            logger.debug(f"Invalid control settings message: {data}")

    def _default_on_command(self, data):
        logger.debug(f"Command message:{data}")
        if data.get("mode", 'def') not in ["manual" , "auto"]:
            logger.debug(f"Invalid command mode: {data.get("mode", 'Not defined')}")
        else:
            if data.get("cmd", 'def') == 'stop':
                # plain stop function (stop here -> full stop)
                logger.debug(f"Calling send stop UDP handler")
                self.udp_server.send_stop(destination=self.destination_robot)

            elif data.get("cmd", 'def') == 'drive':
                if data.get("argc", 'def') in ['forward', 'reverse', 'left', 'right' , 'stop']:
                    # correct manual drive command (stop here -> stop driving)
                    logger.debug(f"Calling send drive UDP handler")
                    self.udp_server.send_drive(destination=self.destination_robot, direction=data.get("argc"))
                else:
                    logger.debug(f"Invalid drive command: {data}")
            else:
                logger.debug(f"Invalid command: {data}")

    def _default_on_set_property(self, data):
        logger.debug(f"Property set message: {data}")
        check = False
        if data.get("cmd", 'def') in ['speed', 'lights', 'horn']:
            tmp_argc = data.get("argc", 'def')
            if tmp_argc in range(0,101) or tmp_argc in ['ON', 'OFF', 'PLAY', 'STOP']:
                # correct set property message
                check = True
        if check:
            logger.debug(f"Calling set property UDP handler")
            self.udp_server.send_set_property(destination=self.destination_robot, property=data.get("cmd"), value=data.get("argc"))
        else:
            logger.debug(f"Invalid set property message: {data}")

    def _default_on_get_property(self, data):
        logger.debug(f"Property get message: {data}")
        check = False
        if data.get("cmd", 'def') in ['speed', 'lights', 'horn']:
            # correct get property message
            check = True

        if check:
            logger.debug(f"Calling get property UDP handler")
            self.udp_server.send_get_property(destination=self.destination_robot, property=data.get("cmd"))
        else:
            logger.debug(f"Invalid get property message: {data}")
