# 🩺 Smart Wearable Medical Monitor (ESP32-C3)

[🦁☀️ برای مطالعه توضیحات به زبان فارسی کلیک کنید](README-fa.md)

![Version](https://img.shields.io/badge/Version-28.0_Pro-blue.svg)
![Platform](https://img.shields.io/badge/Platform-ESP32--C3-green.svg)
![UI](https://img.shields.io/badge/UI-Material_You_3-red.svg)

An advanced, commercial-grade open-source wearable medical gadget designed for real-time health monitoring, smart fall detection, and emergency SOS alerts. Built with the powerful ESP32-C3, it features a fully responsive, iOS-style **Captive Portal** web dashboard.

## ✨ Key Features

- **❤️ Deep Tissue Pulse Oximetry:** Custom-engineered `MAX30102` configuration (Anti-Saturation, 400Hz sample rate, 8192 ADC range) optimized specifically for accurate reading through thick arm/wrist tissue. No more beat skipping!
- **🤸 Smart Fall Detection:** Powered by the `GY-25` (MPU6050) module. Uses a dynamic delta-acceleration algorithm alongside a 6-second consciousness check to eliminate false alarms completely.
- **😰 Panic Attack Detection:** Intelligently monitors abnormal heart rate spikes combined with physical immobility to detect severe stress or panic attacks.
- **🚨 Multi-Function SOS Button:** 
  - **1 Click:** Cancels active alarms (and sends a cancellation SMS).
  - **5 Quick Clicks:** Triggers a manual SOS emergency SMS.
- **📱 True Captive Portal:** No app required! Connect to the gadget's WiFi and the dashboard pops up automatically on your iOS/Android device (bypassing OS restrictions via a local `8.8.8.8` DNS trick).
- **🎨 Beautiful UI:** Dual-language support (EN/FA), Dark/Light mode toggle, and a sleek iOS-style capsule navigation bar.
- **✉️ SMS Alerts:** Real-time emergency dispatch with precise event logging via the `SIM800C` module.

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

## 🚀 How to Use (Captive Portal Magic)

1. Power on the device. (Hold the button during boot if you want to force AP config mode).
2. Search for WiFi networks on your smartphone and connect to **Heart_Monitor**.
3. *Do not open any browser!* Wait a few seconds, and your phone will automatically pop up the medical dashboard.
4. Navigate to the **Settings ⚙️** tab to configure your emergency mobile number, WiFi password, and customized medical thresholds.

## 📚 Libraries Required

Ensure you have the following libraries installed in your Arduino IDE before compiling:
- `SparkFun MAX3010x Pulse and Proximity Sensor Library` (Make sure to include your custom `heartRate.h`)
- `Preferences` (ESP32 Built-in)
- `WiFi`, `WebServer`, `DNSServer` (ESP32 Built-in)

---
*Made with ❤️ by **Mio so...***
