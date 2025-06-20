# Sensi Deauth Panel



**Sensi Deauth Panel** is a multi-purpose, mobile-friendly ESP32-based tool for network testing and security education. It hosts a web interface on its own WiFi network, allowing you to control all its features directly from a mobile phone browser without needing a PC or any special apps.

---

## 🛡️ Legal & Ethical Warning

**This project is intended strictly for educational purposes and for testing on networks and devices that you own or have explicit permission to test.** Unauthorized scanning of networks, launching denial-of-service attacks (like WiFi Deauthentication or Bluetooth flooding) is illegal in most countries. The user of this software is solely responsible for their actions. The developers assume no liability and are not responsible for any misuse or damage caused by this program.

---

## ✨ Features

-   **Standalone & Mobile-First:** No PC or app required. Just connect to the ESP32's WiFi from your phone.
-   **Captive Portal:** Automatically opens the control panel when you connect to the WiFi.
-   **Static IP:** Runs a SoftAP at the fixed IP address `192.168.69.1`.
-   **WiFi Scanner:** Scans for nearby 2.4GHz WiFi networks and displays their SSID, signal strength (RSSI), and channel.
-   **WiFi Deauth Attack:** Sends 802.11 deauthentication frames to a selected Access Point, disconnecting all clients connected to it.
-   **Bluetooth LE Jammer:** Floods the 2.4GHz spectrum with BLE advertising packets with random device names (e.g., "AirPods_XXXX"), making it difficult for other BLE devices to connect.
-   **Real-time Interface:** The web UI provides live status updates (uptime, current mode) and a log panel fed by WebSockets.
-   **Minimalist Dark UI:** Clean, responsive, and easy-to-use interface designed for mobile screens.

---

## 🔧 Hardware Required

-   An ESP32 Development Board (e.g., ESP32-WROOM-32 DevKitC)
-   A Micro-USB cable
-   A power source (power bank, USB wall adapter, or your phone)

### Circuit Diagram

No external wiring is needed. The ESP32 board is all you need.

```
+-----------------+      (Power & Data)      +-----------------+
|  ESP32 Dev Kit  | <----------------------> | USB Cable       |
|    (WROOM-32)   |                          +-----------------+
+-----------------+                                    |
                                                     (USB-OTG Adapter if using a phone)
                                                       |
                                             +-----------------+
                                             |  Android Phone  |
                                             +-----------------+
```

---

## 📲 How to Flash (Using Only a Mobile Phone)

You can flash this project onto your ESP32 using an Android phone and a USB-OTG adapter.

1.  **Install ArduinoDroid:**
    Go to the Google Play Store and install the **[ArduinoDroid - Arduino/ESP8266/ESP32 IDE](https://play.google.com/store/apps/details?id=name.antonsmirnov.android.arduinodroid)** app.

2.  **Add ESP32 Board Support:**
    -   Open ArduinoDroid, go to `≡ (Menu)` > `Settings` > `Board type` > `ESP32`.
    -   In the `Boards manager URLs` field, add this URL: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
    -   Go back and tap `Boards Manager`. Search for `esp32` and install the package by `Espressif Systems`.

3.  **Install Libraries:**
    -   Go to `≡ (Menu)` > `Libraries`.
    -   Search for and install the following libraries:
        -   `ESPAsyncWebServer` (by me-no-dev)
        -   `AsyncTCP` (by me-no-dev)

4.  **Load the Code:**
    -   Download the `SensiDeauthPanel.ino` file to your phone.
    -   In ArduinoDroid, go to `≡ (Menu)` > `Sketch` > `Open` and navigate to the `.ino` file you downloaded.

5.  **Flash the ESP32:**
    -   Connect your ESP32 to your Android phone using a USB-OTG adapter. A notification should appear asking for USB permission; allow it.
    -   In ArduinoDroid, go to `≡ (Menu)` > `Settings` > `Board type` > `ESP32` and select `ESP32 Dev Module`.
    -   Tap the **Upload** button (the arrow pointing down: `↓`).
    -   The app will compile the sketch. If it's the first time, this may take a few minutes.
    -   After compiling, it will start uploading. You may need to **hold the `BOOT` button** on your ESP32 when the "Connecting..." message appears in the console. Release it once the upload begins.
    -   Once finished, the ESP32 will reboot.

---

## 🚀 How to Use the Panel

1.  **Connect to the ESP32:**
    -   Power on your ESP32.
    -   On your phone or laptop, open your WiFi settings.
    -   Connect to the WiFi network named **`Sensi Deauth Panel`**.
    -   The password is **`password123`**.

2.  **Open the Control Panel:**
    -   After connecting, your device should automatically detect the Captive Portal and a notification will pop up to "Sign in to network". Tap it.
    -   The Sensi Deauth Panel interface will load directly in your browser.
    -   If it doesn't open automatically, manually open a browser and go to `http://192.168.69.1`.

3.  **Using the Features:**
    -   **WiFi Scan:** Tap the "Scan" button. A list of nearby networks will populate the dropdown.
    -   **Deauth Attack:** Select a network from the list and tap "Start Deauth Attack".
    -   **BLE Spam:** Tap "Start BLE Spam" to begin the Bluetooth flood.
    -   **Stop:** The "Stop All Attacks" button will halt any active operation.
    -   **Logs:** The log panel at the bottom shows real-time feedback from the ESP32.
