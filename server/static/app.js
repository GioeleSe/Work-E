const ESP_HOSTNAME = "192.168.137.50"
const SERVER_IP = window.location.hostname;
var socket;

const SOCKET_PACKET_STRUCTURE = {
    "endpoint": "",
    "mode": "",
    "cmd": "",
    "argc": ""
};

var main_container_div = null;
var body_div = null;

const ERROR_TYPES = Object.freeze({
    CONN_ERR: Symbol("Connection error"),
    DOM_ERR: Symbol("DOM error"),
});

const hard_dom_error = '<div class="container container-error d-none" id="container-error"><h1 id="error-type">%error type% Error: <span class="conn-error-text error-description" id="error-description">%error description%</span></h1><h4>Verbose Log: <span class="conn-error-text error-log" id="error-log">%verbose log%</span></h4></div>';

// --- Accessory state (mirrors ESP32 defaults) ---
const CLAW_ANGLE_MIN = 25;
const CLAW_ANGLE_MAX = 90;
const LEVER_ANGLE_MIN = 5;
const LEVER_ANGLE_MAX = 74;
const SERVO_STEP = 5;

var claw_angle  = CLAW_ANGLE_MAX;   // start closed
var lever_angle = LEVER_ANGLE_MIN;  // start raised

// --- Radar state ---
var radar_readings = [];  // [{angle, dist}, ...]

function app_init() {
    main_container_div = document.getElementById("main-container");
    body_div = document.getElementById("body");

    try {
        socket = io();
    } catch (e) {
        display_error(ERROR_TYPES.CONN_ERR, "Client Library Error", e);
    }

    socket.on("connect", socket_init);
    socket.on("response", socket_message);

    socket.on("accessory_update", function(data) {
        if (data.claw_angle !== undefined) {
            claw_angle = data.claw_angle;
            document.getElementById("claw-angle-display").innerText = claw_angle + "°";
        }
        if (data.lever_angle !== undefined) {
            lever_angle = data.lever_angle;
            document.getElementById("lever-angle-display").innerText = lever_angle + "°";
        }
    });

    socket.on("radar_scan_result", function(data) {
        if (data.readings && Array.isArray(data.readings)) {
            radar_readings = data.readings;
            radar_draw(radar_readings);
            let minDist = Math.min(...data.readings.map(r => r.dist).filter(d => d > 0));
            let statusEl = document.getElementById("radar-status");
            if (minDist < 400) {
                statusEl.innerText = "Obstacle: " + minDist + " mm";
                statusEl.style.color = "#ff0044";
            } else {
                statusEl.innerText = "Clear (" + minDist + " mm min)";
                statusEl.style.color = "#3a86ff";
            }
        }
    });

    socket.on("connect_error", (err) => {
        console.error("Connection failed", err);
        display_error(ERROR_TYPES.CONN_ERR, "Connection Error", "Socket error: " + err.message);
    });

    socket.on("disconnect", (reason) => {
        console.warn("Socket closed:", reason);
    });
}

function socket_init() {
    console.info("Socket connected");
    try {
        document.getElementById("connecting-container").classList.add("d-none");
        document.getElementById("main-container").classList.remove("d-none");
        document.getElementById("left-container").classList.remove("d-none");
        document.getElementById("right-container").classList.remove("d-none");
        set_robot(document.getElementById("robot-selector").value);
        radar_draw([]);
    } catch (e) {
        display_error(ERROR_TYPES.CONN_ERR, "Client Library Error on Hello Server procedure", e);
    }
}

function socket_message(data) {
    console.info("Message from server:", data);
}

function socket_send(message) {
    console.debug("Sending message", message);
    try {
        socket.emit(message.endpoint, {
            mode: message.mode,
            cmd: message.cmd,
            argc: message.argc
        });
    } catch (e) {
        console.warn("Error while sending message", e);
    }
}

function display_error(error_type, error_description, error_log){
    let error_type_str = "";
    switch (error_type) {
        case ERROR_TYPES.CONN_ERR:
            error_type_str = "Connection";
            let div = document.getElementById("container-error");
            if (div != null) {
                div.classList.remove("d-none");
            } else {
                // fallback: inject the error block into body
                document.getElementById("body").innerHTML = hard_dom_error;
            }
            break;
        case ERROR_TYPES.DOM_ERR:
        default:
            error_type_str = "DOM";
            error_description = "DOM Content Loading Failed. Check the connection and try reloading the page";
            document.getElementById("body").innerHTML = hard_dom_error;
            break;
    }
    let el_type = document.getElementById("error-type");
    let el_desc = document.getElementById("error-description");
    let el_log  = document.getElementById("error-log");
    if (el_type) el_type.firstChild.textContent = error_type_str + " Error: ";
    if (el_desc) el_desc.textContent = String(error_description);
    if (el_log)  el_log.textContent  = String(error_log);
}

function set_robot(controlled_robot){
    console.info("Setting destination robot to ", controlled_robot, " (-1 = none, 0 = all)");
    let msg = { ...SOCKET_PACKET_STRUCTURE };
    let argc;
    try {
        argc = Number(controlled_robot)
    } catch (e) {
        console.warn("invalid controlled robot value");
        console.debug(e);
        argc = -2
    }
    msg.mode = "settings";
    msg.endpoint = "control_settings";
    msg.cmd = "destination_robot";
    msg.argc = argc;
    socket_send(msg);
}

function emergency_stop(){
    console.info("Emergency stop requested");
    let msg = { ...SOCKET_PACKET_STRUCTURE };
    msg.mode = "manual";
    msg.endpoint = 'command';
    msg.cmd = "stop";
    msg.argc = null;
    socket_send(msg);
}

function drive(direction) {
    console.info("Executing drive", direction);
    let msg = { ...SOCKET_PACKET_STRUCTURE };
    msg.mode = "manual";
    msg.endpoint = 'command';
    msg.cmd = "drive";
    msg.argc = direction;
    socket_send(msg);
}

function set_property(feature, action) {
    console.info("Setting property", feature, "to value", action);
    let msg = { ...SOCKET_PACKET_STRUCTURE };
    msg.mode = "manual";
    msg.endpoint = 'set_property';

    switch (feature) {
        case "speed":
            msg.cmd = "speed";
            msg.argc = Number(action);
            break;
        case "lights":
            msg.cmd = "lights";
            msg.argc = action ? 1 : 0;
            break;
        case "brushes":
            msg.cmd = "brushes";
            msg.argc = action ? 1 : 0;
            break;
        case "horn":
            msg.cmd = "horn";
            msg.argc = Number(action);
            break;
        default:
            msg = null;
    }
    msg != null ? socket_send(msg) : console.warn("Unrecognized feature.");
}

// --- Claw control ---
function claw_step(direction) {
    if (direction === 'open') {
        claw_angle = Math.max(claw_angle - SERVO_STEP, CLAW_ANGLE_MIN);
    } else {
        claw_angle = Math.min(claw_angle + SERVO_STEP, CLAW_ANGLE_MAX);
    }
    document.getElementById("claw-angle-display").innerText = claw_angle + "°";
    console.info("Claw step", direction, "→ angle", claw_angle);

    let msg = { ...SOCKET_PACKET_STRUCTURE };
    msg.mode = "manual";
    msg.endpoint = 'command';
    msg.cmd = "claw";
    msg.argc = direction;
    socket_send(msg);
}

// --- Lever control ---
function lever_step(direction) {
    if (direction === 'up') {
        lever_angle = Math.max(lever_angle - SERVO_STEP, LEVER_ANGLE_MIN);
    } else {
        lever_angle = Math.min(lever_angle + SERVO_STEP, LEVER_ANGLE_MAX);
    }
    document.getElementById("lever-angle-display").innerText = lever_angle + "°";
    console.info("Lever step", direction, "→ angle", lever_angle);

    let msg = { ...SOCKET_PACKET_STRUCTURE };
    msg.mode = "manual";
    msg.endpoint = 'command';
    msg.cmd = "lever";
    msg.argc = direction;
    socket_send(msg);
}

// --- Radar canvas ---
function radar_draw(readings) {
    const canvas = document.getElementById('radar-canvas');
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    const W = canvas.width;
    const H = canvas.height;
    const cx = W / 2;
    const cy = H - 8;
    const maxR = H - 18;
    const maxDist = 2000; // mm

    ctx.clearRect(0, 0, W, H);

    // Background fill
    ctx.fillStyle = '#12151f';
    ctx.beginPath();
    ctx.arc(cx, cy, maxR, Math.PI, 0);
    ctx.lineTo(cx, cy);
    ctx.closePath();
    ctx.fill();

    // Range rings at 25 / 50 / 75 %
    for (let frac of [0.25, 0.5, 0.75]) {
        ctx.beginPath();
        ctx.strokeStyle = '#2d323e';
        ctx.lineWidth = 1;
        ctx.arc(cx, cy, maxR * frac, Math.PI, 0);
        ctx.stroke();
    }

    // Outer arc
    ctx.beginPath();
    ctx.strokeStyle = '#444';
    ctx.lineWidth = 1;
    ctx.arc(cx, cy, maxR, Math.PI, 0);
    ctx.stroke();

    // Baseline
    ctx.beginPath();
    ctx.strokeStyle = '#444';
    ctx.moveTo(cx - maxR, cy);
    ctx.lineTo(cx + maxR, cy);
    ctx.stroke();

    // Angle guide lines every 30°
    for (let deg of [30, 60, 90, 120, 150]) {
        let rad = (180 - deg) * Math.PI / 180;
        ctx.beginPath();
        ctx.strokeStyle = '#2d323e';
        ctx.moveTo(cx, cy);
        ctx.lineTo(cx + maxR * Math.cos(rad), cy - maxR * Math.sin(rad));
        ctx.stroke();
    }

    if (readings.length === 0) {
        ctx.fillStyle = '#555';
        ctx.font = '11px Segoe UI';
        ctx.textAlign = 'center';
        ctx.fillText('No data', cx, cy - maxR / 2);
    } else {
        for (let r of readings) {
            if (r.dist <= 0 || r.dist > maxDist) continue;
            let ratio = r.dist / maxDist;
            let rad = (180 - r.angle) * Math.PI / 180;
            let px = cx + maxR * ratio * Math.cos(rad);
            let py = cy - maxR * ratio * Math.sin(rad);

            ctx.beginPath();
            if (r.dist < 400) {
                ctx.fillStyle = '#ff0044';
            } else if (r.dist < 800) {
                ctx.fillStyle = '#ffa500';
            } else {
                ctx.fillStyle = '#3a86ff';
            }
            ctx.arc(px, py, 3, 0, 2 * Math.PI);
            ctx.fill();
        }
    }

    // Robot indicator
    ctx.beginPath();
    ctx.fillStyle = '#e0e0e0';
    ctx.arc(cx, cy, 4, 0, 2 * Math.PI);
    ctx.fill();
}

document.addEventListener('DOMContentLoaded', app_init);
