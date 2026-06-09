<<<<<<< Updated upstream
const ESP_HOSTNAME = "192.168.137.50"
const SERVER_IP = window.location.hostname; 
var socket; 

const SOCKET_PACKET_STRUCTURE = {
    "endpoint": "",
    "mode": "",
    "cmd": "",
    "argc": ""
=======
const SERVER_IP = window.location.hostname;
var socket;

const SOCKET_PACKET_STRUCTURE = {
    endpoint: "",
    mode: "",
    cmd: "",
    argc: ""
>>>>>>> Stashed changes
};

var main_container_div = null;
var body_div = null;

const ERROR_TYPES = Object.freeze({
    CONN_ERR: Symbol("Connection error"),
    DOM_ERR: Symbol("DOM error"),
});

<<<<<<< Updated upstream
const hard_dom_error = '<div class="container container-error d-none" id="container-error"><h1 id="error-type">%error type% Error: <span class="conn-error-text error-description" id="error-description">%error description%</span></h1><h4>Verbose Log: <span class="conn-error-text error-log" id="error-log">%verbose log%</span></h4></div>';
=======
const hard_dom_error =
    '<div class="container container-error d-none" id="container-error">' +
    '<h1 id="error-type">%error type% Error: ' +
    '<span class="conn-error-text error-description" id="error-description">%error description%</span></h1>' +
    '<h4>Verbose Log: <span class="conn-error-text error-log" id="error-log">%verbose log%</span></h4></div>';
>>>>>>> Stashed changes

function app_init() {
    main_container_div = document.getElementById("main-container");
    body_div = document.getElementById("body");

    try {
<<<<<<< Updated upstream
        socket = io(); 
=======
        socket = io();
>>>>>>> Stashed changes
    } catch (e) {
        display_error(ERROR_TYPES.CONN_ERR, "Client Library Error", e);
    }

<<<<<<< Updated upstream
    // --- Socket.io Event Listeners ---
    socket.on("connect", socket_init);
    
    socket.on("response", socket_message); // Matches the 'emit' from Python
=======
    socket.on("connect", socket_init);
    socket.on("response", socket_message);
>>>>>>> Stashed changes

    socket.on("connect_error", (err) => {
        console.error("Connection failed", err);
        display_error(ERROR_TYPES.CONN_ERR, "Connection Error", "Socket error: " + err.message);
    });

<<<<<<< Updated upstream
=======
    socket.on("robot_heartbeat", data => socket_message("heartbeat", data));
    socket.on("robot_event", data => socket_message("event", data));
    socket.on("robot_feedback", data => socket_message("feedback", data));
    socket.on("robot_error", data => socket_message("error", data));

>>>>>>> Stashed changes
    socket.on("disconnect", (reason) => {
        console.warn("Socket closed:", reason);
    });
}

function socket_init() {
    console.info("Socket connected");
<<<<<<< Updated upstream
    try{
=======

    try {
>>>>>>> Stashed changes
        document.getElementById("connecting-container").classList.add("d-none");
        document.getElementById("main-container").classList.remove("d-none");
        document.getElementById("left-container").classList.remove("d-none");
        document.getElementById("right-container").classList.remove("d-none");
<<<<<<< Updated upstream
        set_robot(document.getElementById("robot-selector").value);
    }catch (e) {
        display_error(ERROR_TYPES.CONN_ERR, "Client Library Error on Hello Server procedure", e);
    }
}

function socket_message(data) {
    console.info("Message from server:", data);
}

function socket_send(message) {
    console.debug("Sending message", message);
    try {
        // Socket.io handles JSON.stringify for you automatically
        // We emit to the specific 'command' event defined in Python
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
            error_type_str = "Connection error";
            let div  = document.getElementById("container-error");
            if(div != null){
                div.classList.remove("d-none");
            }else{
                error_log = "container-error not found. Unsafe DOM content.";
                display_error(ERROR_TYPES.DOM_ERR, "", error_log);
            }
            break;
        case ERROR_TYPES.DOM_ERR:
        default:
            error_type_str = "DOM error";
            error_description = "DOM Content Loading Failed. Check the connection and try reloading the page";
            body_div = document.getElementById("body");
            body_div.innerHTML = hard_dom_error;
            break;
    }
    body_div.getElementById("error-type").innertext = error_type_str;
    body_div.getElementById("error-description").innertext = error_description;
    body_div.getElementById("error-log").innertext = error_log;
    body_div.getElementById("error-container").classList.remove("d-none");
}

function set_robot(controlled_robot){
    console.info("Setting destination robot to ", controlled_robot, " (-1 = none, 0 = all)");
    let msg = { ...SOCKET_PACKET_STRUCTURE };
    let argc;
    try{
        argc = Number(controlled_robot)
    }catch (e){
        console.warn("invalid controlled robot value");
        console.debug(e);
        argc = -2
    }
=======
        // set_robot(document.getElementById("robot-selector").value); // this was the function to set the robot_id
        const urlParams = new URLSearchParams(window.location.search);
        const targetRobotId = urlParams.get('robot_id');

        if (targetRobotId !== null) {
            // If query param exists, use it!
            set_robot(targetRobotId);
        } else {
            // Fallback default or read from your old selector if it exists on the page
            const selector = document.getElementById("robot-selector");
            if (selector) {
                set_robot(selector.value);
            } else {
                console.warn("No robot_id found in URL. Defaulting to robot 1.");
                set_robot(1);
            }
        }
    } catch (e) {
        display_error(ERROR_TYPES.CONN_ERR, "Client Init Error", e);
    }
}

function socket_message(type, data) {

    switch (data.event) {

        case "radar_scan_min":
            add_debug_message(
                "feedback",
                `Robot ${data.robot_id}: Radar got closest obstacle at ${data.min_distance_mm} mm`
            );
            break;

        case "radar_scan":
            add_debug_message(
                "feedback",
                `Robot ${data.robot_id}: Radar got ${data.distance_mm} mm at angle ${data.angle}`
            );
            break;

        case "load_collected":
            add_debug_message(
                "event",
                `Robot ${data.robot_id}: Acquired payload`
            );
            break;

        case "load_released":
            add_debug_message(
                "event",
                `Robot ${data.robot_id}: Released payload`
            );
            break;

        default:
            add_debug_message(
                type,
                JSON.stringify(data)
            );
    }
}

function add_debug_message(type, text) {
    const log = document.getElementById("debug-log");

    if (!log) {
        console.warn("debug-log container not found");
        return;
    }

    const isScrolledToBottom = log.scrollHeight - log.clientHeight - log.scrollTop < 30;

    const row = document.createElement("div");
    row.classList.add("log-entry");
    row.classList.add(`log-${type}`);

    const now = new Date().toLocaleTimeString();
    row.textContent = `[${now}] ${text}`;

    // Change to appendChild so new logs go to the bottom
    log.appendChild(row);

    // Keep memory clean: remove the oldest item (which is now the first child)
    while (log.children.length > 100) {
        log.removeChild(log.firstChild);
    }

    // Only force auto-scroll if the user wasn't actively looking at older logs
    if (isScrolledToBottom) {
        log.scrollTop = log.scrollHeight;
    }
}
function socket_send(message) {
    console.log("--- socket_send Intercept Start ---");
    console.log("Raw incoming message object:", message);

    if (!message || !message.endpoint) {
        console.warn("Invalid socket message or missing endpoint:", message);
        return;
    }

    let dynamicRobotId = null;

    // Strategy 1: Check URL Parameters
    const urlParams = new URLSearchParams(window.location.search);
    const urlRobotId = urlParams.get('robot_id');
    console.log(`Strategy 1 (URL param 'robot_id'):`, urlRobotId);

    if (urlRobotId !== null) {
        dynamicRobotId = Number(urlRobotId);
        console.log(`Parsed dynamicRobotId from URL as type number:`, dynamicRobotId);
    } else {
        // Strategy 2: Fallback to the HTML body attribute if URL doesn't have it
        const bodyEl = document.getElementById("body");
        if (!bodyEl) {
            console.log("Strategy 2 Check failed: HTML element with id='body' was not found in the DOM.");
        }

        const bodyRobotId = bodyEl ? bodyEl.getAttribute('data-robot-id') : null;
        console.log(`Strategy 2 (HTML attribute 'data-robot-id'):`, bodyRobotId);

        if (bodyRobotId !== null) {
            dynamicRobotId = Number(bodyRobotId);
            console.log(`Parsed dynamicRobotId from HTML Attribute as type number:`, dynamicRobotId);
        }
    }

    // Construct the payload
    const payload = {
        robot_id: (dynamicRobotId !== null && !isNaN(dynamicRobotId)) ? dynamicRobotId : -1,
        mode: message.mode,
        cmd: message.cmd,
        argc: message.argc
    };

    console.log("Final compiled payload being sent to server:", payload);

    let UI_log_text = `UI Command: endpoint='${message.endpoint}' | cmd='${message.cmd}'`;
    if (message.argc !== null && message.argc !== undefined && message.argc !== "") {
        UI_log_text += ` | value=${message.argc}`;
    }
    add_debug_message("event", UI_log_text);

    try {
        console.log(`Emitting event '${message.endpoint}' via socket.io...`);
        socket.emit(message.endpoint, payload);
    } catch (e) {
        console.warn("Transmission failure encountered during socket emit:", e);
    }
    console.log("--- socket_send Intercept End ---");
}

function display_error(error_type, error_description, error_log) {
    let error_type_str = "";

    switch (error_type) {

        case ERROR_TYPES.CONN_ERR:
            error_type_str = "Connection error";

            let div = document.getElementById("container-error");

            if (div != null) {
                div.classList.remove("d-none");
            } else {
                display_error(ERROR_TYPES.DOM_ERR, "", "container-error not found");
                return;
            }
            break;

        case ERROR_TYPES.DOM_ERR:
        default:
            error_type_str = "DOM error";
            error_description = "DOM Content Loading Failed. Reload page";

            if (body_div) {
                body_div.innerHTML = hard_dom_error;
            }
            break;
    }
    const elType = document.getElementById("error-type");
    const elDesc = document.getElementById("error-description");
    const elLog = document.getElementById("error-log");

    if (elType && elDesc && elLog) {
        elType.innerText = error_type_str;
        elDesc.innerText = error_description;
        elLog.innerText = error_log;
    } else {
        console.error(`UI Error Elements missing! Context: ${error_type_str} - ${error_description}`);
        alert(`${error_type_str}: ${error_description}\n\nLog: ${error_log}`); // fall back to a basic alert or console log
    }
    document.getElementById("container-error").classList.remove("d-none");
}

function set_robot(controlled_robot) {
    console.info("Setting destination robot to", controlled_robot);

    let n = Number(controlled_robot);
    let argc = Number.isFinite(n) ? n : -2;

    let msg = {...SOCKET_PACKET_STRUCTURE};
>>>>>>> Stashed changes
    msg.mode = "settings";
    msg.endpoint = "control_settings";
    msg.cmd = "destination_robot";
    msg.argc = argc;
<<<<<<< Updated upstream
    socket_send(msg);
}

function emergency_stop(){
    console.info("Emergency stop requested");
    let msg = { ...SOCKET_PACKET_STRUCTURE };
    msg.mode = "manual";
    msg.endpoint = 'command';
    msg.cmd = "stop";
    msg.argc = null;
=======

    socket_send(msg);
}

function emergency_stop() {
    console.info("Emergency stop requested");

    let msg = {...SOCKET_PACKET_STRUCTURE};
    msg.mode = "manual";
    msg.endpoint = "command";
    msg.cmd = "stop";
    msg.argc = null;

>>>>>>> Stashed changes
    socket_send(msg);
}

function drive(direction) {
<<<<<<< Updated upstream
    console.info("Executing drive ", direction);
    let msg = { ...SOCKET_PACKET_STRUCTURE };
    msg.mode = "manual";
    msg.endpoint = 'command';
    msg.cmd = "drive";
    msg.argc = direction;
=======
    console.info("Executing drive", direction);

    let msg = {...SOCKET_PACKET_STRUCTURE};
    msg.mode = "manual";
    msg.endpoint = "command";
    msg.cmd = "drive";
    msg.argc = direction;

>>>>>>> Stashed changes
    socket_send(msg);
}

function set_property(feature, action) {
<<<<<<< Updated upstream
    console.info("Setting property ", feature, " to value ", action);
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
            msg.argc = action?1:0;
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

document.addEventListener('DOMContentLoaded', app_init);
=======
    console.info("Setting property", feature, action);

    let msg = {...SOCKET_PACKET_STRUCTURE};
    msg.mode = "manual";
    msg.endpoint = "set_property";

    switch (feature.toUpperCase()) {

        case "SPEED":
            msg.cmd = "speed";
            msg.argc = Number(action);
            break;

        case "RADAR":
            msg.cmd = "radar";
            msg.argc = Number(action);
            break;

        case "LIGHTS":
            msg.cmd = "lights";
            msg.argc = Number(action);
            break;

        case "HORN":
            msg.cmd = "horn";
            msg.argc = Number(action);
            break;

        case "TRUNK":
            msg.cmd = "OBJECT_UNLOADER";
            msg.argc = Number(action);
            break;

        case "BRUSHES":
            msg.cmd = "OBSTACLE_CLEANER";
            msg.argc = Number(action);
            break;

        case "CLAW":
            msg.cmd = "OBJECT_LOADER";
            msg.argc = Number(action);
            break;

        default:
            console.warn("Unrecognized feature.");
            return;
    }

    socket_send(msg);
}

document.addEventListener("DOMContentLoaded", app_init);
>>>>>>> Stashed changes
