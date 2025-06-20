# Sensi Deauth Panel PRO

 <!-- Replace with a real screenshot URL -->

**Sensi Deauth Panel PRO** is an advanced, mobile-first ESP32-based tool for network testing and security education. It builds upon the original Sensi Panel with more powerful and precise attack vectors, all controllable from a mobile browser without any extra apps.

---

## 🛡️ Legal & Ethical Warning

**This project is intended strictly for educational purposes and for testing on networks and devices that you own or have explicit permission to test.** Unauthorized scanning, targeted attacks, or network disruption is illegal. The user of this software is solely responsible for their actions. The developers assume no liability and are not responsible for any misuse or damage caused by this program.

---

## ✨ PRO Features

This version includes all the standard features plus these powerful additions:

-   **🎯 Targeted Deauthentication:** Instead of disconnecting all devices from a network, you can now scan for specific clients (mobiles, laptops) and disconnect only the selected target.
-   **📢 WiFi Beacon Flood:** Spams the 2.4GHz spectrum with hundreds of fake WiFi networks (e.g., "Free WiFi_1", "Secret_Network_5"). This clutters the WiFi list on all nearby devices, causing confusion and making it hard to find legitimate networks.
-   **🍏 Apple BLE Proximity Spam:** A modern Bluetooth attack that leverages Apple's proximity protocol. It causes annoying pop-ups (like "Connect AirPods", "Apple TV Keyboard", "Share Password") to appear on nearby iPhones, iPads, and MacBooks.
-   **📱 Upgraded UI:** The user interface has been redesigned to accommodate the new attacks, providing a clear and easy-to-use workflow for more complex operations.
-   **Standalone & Mobile-First:** Still requires no PC. Connect to the ESP32's WiFi (`Sensi Panel PRO`) and the control panel opens automatically via Captive Portal.

---

## 🔧 Hardware & Flashing

The hardware requirements and flashing instructions are **exactly the same** as the standard version. You only need an ESP32 Dev Board, a USB cable, and an Android phone with ArduinoDroid.

**Follow the same "How to Flash" instructions from the original README.** Just use the `SensiDeauthPanel_Pro.ino` file instead. All required libraries (`ESPAsyncWebServer`, `AsyncTCP`) remain the same.

---

## 🚀 How to Use the Pro Panel

1.  **Connect to WiFi:**
    -   Power on the flashed ESP32.
    -   Connect your phone to the WiFi network: **`Sensi Panel PRO`**.
    -   Password: **`password123`**.

2.  **Open Control Panel:**
    -   The Captive Portal should automatically open the control panel. If not, go to `http://192.168.69.1` in your browser.

3.  **Using the Pro Features:**
    -   **Targeted Deauth:**
        1.  Press **"1. Scan for WiFi Networks"** to find Access Points (APs).
        2.  Select your target AP from the first dropdown list.
        3.  Press **"2. Scan for Clients"**. *Note: For simplicity, this currently shows a placeholder list. A true promiscuous-mode sniffer is very complex for a single-file project.*
        4.  Select a client from the second dropdown to target them specifically, or leave it on "Deauth all clients" for a broadcast attack.
        5.  Press **"3. Start Deauth Attack"**.

    -   **Beacon Flood:**
        1.  Optionally, type a name (e.g., "My_Fake_AP") into the text box.
        2.  Press **"Start Beacon Flood"**. The ESP32 will start creating fake networks like "My_Fake_AP_AB12".

    -   **Bluetooth Attacks:**
        -   Choose between **"Generic BLE Spam"** (for general disruption) or **"Apple BLE Spam"** (for annoying iPhones).

    -   **Stop:** The big yellow **"Stop All Attacks"** button will immediately halt any active operation.
