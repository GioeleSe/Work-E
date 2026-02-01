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

function app_init() {
    main_container_div = document.getElementById("main-container");
    body_div = document.getElementById("body");

    try {
        socket = io(); 
    } catch (e) {
        display_error(ERROR_TYPES.CONN_ERR, "Client Library Error", e);
    }

    // --- Socket.io Event Listeners ---
    socket.on("connect", socket_init);
    
    socket.on("response", socket_message); // Matches the 'emit' from Python

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
    try{
        document.getElementById("connecting-container").classList.add("d-none");
        document.getElementById("main-container").classList.remove("d-none");
        document.getElementById("left-container").classList.remove("d-none");
        document.getElementById("right-container").classList.remove("d-none");
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
    console.info("Executing drive ", direction);
    let msg = { ...SOCKET_PACKET_STRUCTURE };
    msg.mode = "manual";
    msg.endpoint = 'command';
    msg.cmd = "drive";
    msg.argc = direction;
    socket_send(msg);
}

function set_property(feature, action) {
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
            msg.argc = action?'ON':'OFF';
            break;
        case "horn":
            msg.cmd = "horn";
            msg.argc = action;
            break;
        default:
            msg = null;
    }
    msg != null ? socket_send(msg) : console.warn("Unrecognized feature.");
}

document.addEventListener('DOMContentLoaded', app_init);