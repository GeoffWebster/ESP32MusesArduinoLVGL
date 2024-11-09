/*
 * ----------------------------------------------------------------------------
 * Preamplifier Remote Control with WebSocket
 * © 2024 Geoff Webster
 * ----------------------------------------------------------------------------
 */

 var gateway = `ws://${window.location.hostname}/ws`;
 var websocket;

// ----------------------------------------------------------------------------
// Initialization
// ----------------------------------------------------------------------------

window.addEventListener('load', onLoad);

function onLoad(event) {
    initWebSocket();
    initButton();
}

// ----------------------------------------------------------------------------
// WebSocket handling
// ----------------------------------------------------------------------------

function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
    let data = JSON.parse(event.data);
    document.getElementById('source').innerHTML = data.source;
    document.getElementById('volume').innerHTML = data.volume;
    if (data.mute == "off")
    {
        document.getElementById('volume').className = "off";
        document.getElementById('muted').className = "on";
    }
    else if (data.mute == "on")
    {
        document.getElementById('volume').className = "on";
        document.getElementById('muted').className = "off";
    }
}
// ----------------------------------------------------------------------------
// Button handling
// ----------------------------------------------------------------------------

function initButton() {
    document.getElementById('phono').addEventListener('click', onPhono);
    document.getElementById('media').addEventListener('click', onMedia);
    document.getElementById('cd').addEventListener('click', onCD);
    document.getElementById('tuner').addEventListener('click', onTuner);
    document.getElementById('voldown').addEventListener('click', onVoldown);
    document.getElementById('volup').addEventListener('click', onVolup);
    document.getElementById('mute').addEventListener('click', onMute);
}

function onPhono(event) {
    websocket.send(JSON.stringify({'Phono':'toggle'}));
}

function onMedia(event) {
    websocket.send(JSON.stringify({'Media':'toggle'}));
}

function onCD(event) {
    websocket.send(JSON.stringify({'CD':'toggle'}));
}
function onTuner(event) {
    websocket.send(JSON.stringify({'Tuner':'toggle'}));
}

function onVoldown(event) {
    websocket.send(JSON.stringify({'Voldown':'toggle'}));
}

function onVolup(event) {
    websocket.send(JSON.stringify({'Volup':'toggle'}));
}

function onMute(event) {
    websocket.send(JSON.stringify({'Mute':'toggle'}));
}