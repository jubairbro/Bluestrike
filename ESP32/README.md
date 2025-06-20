# Sensi Deauth Panel

**Sensi Deauth Panel** is a complete, mobile-accessible ESP32-based "WiFi + Bluetooth Attack Tool" designed strictly for educational and network security testing purposes. It hosts a web server with a user-friendly, responsive interface, allowing all operations to be controlled directly from a mobile phone's browser without needing a PC.

 
*(This is a representative image of the UI)*

---

### 🛡️ Legal & Ethical Disclaimer

This project is intended for **educational purposes only**. The user is solely responsible for their actions. Using this tool on networks or devices you do not own or have explicit permission to test is **illegal**. The developer assumes no liability and is not responsible for any misuse or damage caused by this program. **Always act responsibly and ethically.**

---

### ✨ Features

-   **ESP32 as a Standalone Access Point:** Creates its own Wi-Fi network with a static IP (`192.168.69.1`).
-   **Mobile-First Web UI:** A sleek, dark-themed, and responsive web interface for full control from any mobile browser.
-   **Captive Portal:** Automatically redirects connected devices to the control panel.
-   **WiFi Scanner:** Scans for and lists nearby WiFi networks (SSIDs) with their signal strength (RSSI).
-   **WiFi Deauthentication Attack:** Disconnects all clients from a selected target network.
-   **Bluetooth LE Jammer:** Floods the 2.4GHz spectrum with thousands of fake Bluetooth Low Energy (BLE) device advertisements.
-   **Real-time Status & Logs:** The web UI displays the device's uptime, current mode, and a live log of all operations.
-   **Zero Dependencies:** The entire web interface is embedded in the ESP32's firmware. No need to upload separate HTML/CSS/JS files.
-   **Mobile Flashing Support:** Can be flashed onto an ESP32 using only an Android phone and an OTG cable.

---

### 🔧 Hardware Requirements

1.  ESP32 Dev Board (WROOM-32 or similar)
2.  Micro-USB Cable (for power and flashing)
3.  (For Mobile Flashing) An Android Phone with OTG support.
4.  (For Mobile Flashing) A USB-OTG Adapter to connect the ESP32 to your phone.

### 🔩 Circuit Diagram

No external components are required. Just power up the ESP32 board.

```
+-------------------+
|                   |
|   ESP32 Dev Board |-----> (Micro-USB for Power/Flashing)
|      (WROOM-32)   |
|                   |
+-------------------+
```

---

### 📱 How to Flash Using Only a Mobile Phone (Android)

You can compile and upload this firmware using your Android phone.

**Step 1: Install ArduinoDroid**
- Go to the Google Play Store and install the **[ArduinoDroid - Arduino/ESP8266/ESP32 IDE](https://play.google.com/store/apps/details?id=name.antonsmirnov.android.arduinodroid)** app.

**Step 2: Setup ESP32 Support in ArduinoDroid**
- Open ArduinoDroid.
- Go to **Menu (☰) > Settings > Board type > ESP32**.
- Tap on `esp32` by `Espressif Systems` and wait for the board definitions to be downloaded and installed.

**Step 3: Install Required Libraries**
- You need the `ESPAsyncWebServer` and `AsyncTCP` libraries.
- Go to **Menu (☰) > Libraries > Manage Libraries**.
- Search for and install the following libraries:
    1.  `ESPAsyncWebServer` by `me-no-dev`
    2.  `AsyncTCP` by `me-no-dev`

**Step 4: Load the Code**
- Copy the entire code from the `Sensi_Deauth_Panel.ino` file provided above.
- In ArduinoDroid, open the editor (the main screen).
- Paste the code, replacing any default code.

**Step 5: Connect and Flash**
1.  Connect your ESP32 board to your Android phone using the Micro-USB cable and the USB-OTG adapter.
2.  A popup might appear on your phone asking for permission for ArduinoDroid to access the USB device. Grant it.
3.  In ArduinoDroid, go to **Menu (☰) > Settings > Board type > ESP32** and select **"ESP32 Dev Module"**.
4.  Click the **Compile** button (checkmark icon ✔️) at the top. It should compile without errors.
5.  Click the **Upload** button (down arrow icon ⬇️) to flash the firmware to your ESP32.
6.  You can monitor the upload process in the console output at the bottom of the screen.

---

### 🚀 How to Use the Sensi Deauth Panel

1.  **Power On:** Power your flashed ESP32 using any USB power source (like a power bank or charger).
2.  **Connect to WiFi:** On your mobile phone, open your WiFi settings. Find and connect to the network named **`Sensi Deauth Panel`**. There is no password.
3.  **Open Control Panel:**
    -   After connecting, your phone should automatically detect the Captive Portal and show a "Sign in to network" notification. Tap it.
    -   If it doesn't open automatically, open your mobile browser (Chrome, Safari, etc.) and navigate to the address `192.168.69.1`.
4.  **Operate from the Panel:**
    -   **To Scan WiFi:** Tap the `Scan WiFi Networks` button. The log will show the progress, and the dropdown list will be populated with nearby SSIDs.
    -   **To Deauth:** Select a network from the dropdown and tap `Start Deauth`. The attack will begin. Tap `Stop Attack` to end it.
    -   **To Jam Bluetooth:** Tap `Start BLE Flood`. Tap `Stop Attack` to end it.
    -   Monitor the `Live Logs` box for real-time feedback on all actions.

That's it! You now have a fully portable network testing tool.
