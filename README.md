# NETCIPHER

### *ESP32 Wi-Fi Security Testing Framework*
 
**Platform:** ESP32  

---

## 📡 About NETCIPHER:-

NETCIPHER is a **Wi-Fi security testing framework** for ESP32. It combines powerful pentesting attacks — all controllable via OLED display + buttons and Web Interface.

---

## Features

### Core Attacks

| Attack | Description |
|:---|:---|
| **Deauth Attack** | Force disconnect clients from Wi-Fi networks |
| **Multi-Target Deauth** | Attack multiple APs simultaneously |
| **Deauth Flood** | High-speed packet bursts (500+ packets/sec) |
| **STA Deauth** | Target specific clients |
| **Reactive Deauth/Dynamic Deauth** | Auto-deauth connecting clients to selected AP |

### Spoofing & Capture

| Attack | Description |
|:---|:---|
| **Evil Twin** | Clone any network, capture passwords with captive portal |
| **Hidden SSID Recovery** | Force hidden networks to reveal their SSID themselves |
| **Beacon Spam** | Flood with fake SSIDs (Random) |

### Monitoring & Logging

| Feature | Description |
|:---|:---|
| **Deauth Detector** | Detect deauth attacks in your area |
| **Station Scanner** | Discover clients connected to selected APs |
| **Nearby Scanner** | See nearby Stations |
| **Password Logs** | Persistent storage with verification status |

### Extra features

| Features | Description |
|:---|:---|
| **Web Interface** | Control all actions with web interface |
| **OLED Display** | 128x64 (SH1106) |
| **Custom AP Settings** | Change your NETCIPHER's AP and Password Settings|
| **Physical Buttons** | Full control without phone |
| **Digital Clock** | Real-time clock on OLED(only if any device is connected to softAP) |
| **AC Detector** | Detects live 50/60Hz AC wires via GPIO 34 |

---

## Hardware Requirements

| Component | Recommended |
|:---|:---|
| **Microcontroller** | ESP32 DevKitC |
| **Display** | OLED 128x64 SH1106 |
| **Buttons** | 4x tactile switches |
| **LED** | Built-in or external |
| **Power** | Battery / USB |

### Pin Configuration

| Component | ESP32 Pins |
|:---|:---|
| **OLED (I2C)** | SDA=21, SCL=22 |
| **Button UP** | GPIO 26 |
| **Button DOWN** | GPIO 32 |
| **Button OK** | GPIO 27 |
| **Button BACK** | GPIO 25 |
| **LED** | GPIO 2/Built in |

---

## Getting Started

1. **Download** the latest firmware from Releases
2. **Flash** the `.bin` file to your ESP32 using ESP32 Flash Download Tool or esptool
3. **Connect** OLED and buttons as per pin configuration
5. **Power on** and navigate using the buttons or connect to NETCIPHER wifi and use the web interface

---

## Controls

| Button | Action |
|:---|:---|
| **UP / DOWN** | Navigate menus |
| **OK** | Select / Confirm |
| **BACK** | Return / Exit |

---

## ⚠️ Legal Disclaimer ⚠️

> **NETCIPHER is for educational and authorized security testing purposes only.**
>
> - Only test networks you own or have written permission to test
> - Unauthorized use may violate local, state, and federal laws
> - The developer assumes no liability for misuse

---

## Acknowledgments

- **[Spacehuhn](https://github.com/spacehuhn)** — For the original ESP8266 Deauther that inspired this project
- **ESP32 Community** — For continuous support and inspiration

---
