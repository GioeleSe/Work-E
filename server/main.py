import logging
import sys

# Force the root logger to behave
root = logging.getLogger()
root.setLevel(logging.DEBUG)

# Create a formatter that is very obvious
formatter = logging.Formatter('--- %(levelname)s --- %(name)s --- %(message)s')

# Handler 1: Console
console_handler = logging.StreamHandler(sys.stdout)
console_handler.setFormatter(formatter)
root.addHandler(console_handler)

# Handler 2: File (use a fresh name to avoid Windows locks)
file_handler = logging.FileHandler("debug_dump.log", mode='w')
file_handler.setFormatter(formatter)
root.addHandler(file_handler)

logging.info("LOGGING SYSTEM INITIALIZED")

# NOW YOU IMPORT
from server_udp import UDPServer
from server_http import HTTPServer

SERVING_ALL = "0.0.0.0"
UDP_PROTOCOL_ID = "robot-net/1.0"
UDP_PORT = 8000
HTTP_PORT = 80
FRAME_SIZE = 2048

udp_server = UDPServer(
    bind_ip=SERVING_ALL,
    bind_port=UDP_PORT,
    protocol_id=UDP_PROTOCOL_ID,
    frame_size=FRAME_SIZE,
    localhost_dev=True
)

http_server = HTTPServer(
    host=SERVING_ALL,
    port=HTTP_PORT,
)


if __name__ == "__main__":
    udp_server.link_http_server(http_server)
    http_server.link_udp_server(udp_server)

    udp_server.start()
    http_server.start(threaded=False)