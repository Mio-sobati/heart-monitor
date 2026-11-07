# 🩺 Smart Wearable Medical Monitor (ESP32-C3)

![Version](https://img.shields.io/badge/Version-28.0-blue.svg)
![Platform](https://img.shields.io/badge/Platform-ESP32--C3-green.svg)
![UI](https://img.shields.io/badge/UI-Material_You_3-red.svg)

An advanced, commercial-grade open-source wearable medical gadget designed for real-time health monitoring, smart fall detection, and emergency SOS alerts. Built with ESP32-C3 and features a fully responsive iOS-style Captive Portal web dashboard.
🇮🇷 برای مطالعه توضیحات به زبان فارسی کلیک کنید](README-fa.md)

## ✨ Key Features

- **❤️ Deep Tissue Pulse Oximetry:** Optimized `MAX30102` settings (Anti-Saturation & Peak Skipping filters) for accurate arm/wrist measurements.
- **🤸 Smart Fall Detection:** Uses `GY-25` (MPU6050) with a dynamic delta-acceleration algorithm and a 6-second consciousness check to prevent false alarms.
- **😰 Panic Attack Detection:** Monitors abnormal heart rate spikes combined with immobility.
- **🚨 Multi-Function SOS Button:** 1 click to cancel alarms, 5 quick clicks for manual SMS emergency requests.
- **📱 Captive Portal Dashboard:** No app required! Connect to the gadget's WiFi and the dashboard pops up automatically (iOS/Android).
- **🎨 Beautiful UI:** Dual-language (EN/FA), Dark/Light mode, and an iOS-style capsule navigation bar.
- **✉️ SMS Alerts:** Real-time emergency SMS dispatch via `SIM800C`.

## 🛠️ Hardware Requirements

*   **Microcontroller:** ESP32-C3 Super Mini
*   **Heart Rate Sensor:** MAX30102 (I2C)
*   **Gyroscope:** GY-25 / MPU6050 (Serial)
*   **GSM Module:** SIM800C (Serial)
*   **Actuators:** Active Buzzer & Vibration Motor
*   **Input:** 1x Push Button

## 🔌 Pin Configuration

| Component | Pin (ESP32-C3) | Notes |
| :--- | :--- | :--- |
| **Buzzer** | `GPIO 1` | Active LOW |
| **Vibrator** | `GPIO 21` | Active HIGH |
| **Button** | `GPIO 10` | INPUT_PULLUP |
| **SIM800 TX** | `GPIO 4` | Connect to RX of SIM800 |
| **SIM800 RX** | `GPIO 5` | Connect to TX of SIM800 |
| **GY-25 TX** | `GPIO 2` | Connect to RX of GY-25 |
| **GY-25 RX** | `GPIO 3` | Connect to TX of GY-25 |
| **MAX30102 SDA**| `GPIO 8` | I2C Data |
| **MAX30102 SCL**| `GPIO 9` | I2C Clock |

## 🚀 How to Use (Captive Portal)

1. Power on the device.
2. Search for WiFi networks on your phone and connect to **Heart_Monitor**.
3. *Do not open a browser!* Your phone will automatically pop up the medical dashboard.
4. Go to the **Settings ⚙️** tab to configure your emergency phone number, WiFi password, and medical thresholds.

## 📚 Libraries Required

Ensure you have the following libraries installed in your Arduino IDE:
- `SparkFun MAX3010x Pulse and Proximity Sensor Library`
- `Preferences` (ESP32 Built-in)
- `WiFi`, `WebServer`, `DNSServer` (ESP32 Built-in)

---
*Made with ❤️ by **Mio so...***
