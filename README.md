# ESP32-Midtrans-Payment-Gateway

This repository contains the firmware for a standalone Internet of Things (IoT) digital payment terminal built on the ESP32 platform. The device features a touchscreen interface, integrates with the Midtrans Payment Gateway API to generate dynamic QRIS and Virtual Account (VA) numbers.

## 🛠️ Hardware Requirements

* **ESP32 Development Board**
* **TFT Display** (Compatible with `TFT_eSPI`, e.g., ILI9341)
* **XPT2046 Touchscreen Controller**
* **Status LED & Buzzer/Feedback Actuator**
* **(Optional) Serial Thermal Printer**

### Pin Configuration

| Component | ESP32 Pin | Notes |
| :--- | :--- | :--- |
| **Touch CS** | `21` | SPI Chip Select for XPT2046 |
| **Touch IRQ** | `22` | Interrupt pin for touch detection |
| **Serial2 RX** | `16` | External device communication |
| **Serial2 TX** | `17` | TX out to Thermal Printer |
| **Touch Feedback** | `25` | Toggles HIGH/LOW on touch (Buzzer/Haptic) |
| **Status LED** | `26` | Blinks during connection, solid when connected |
---

## 📦 Software Dependencies

Ensure the following libraries are installed in your Arduino IDE:

* [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) (Requires custom `User_Setup.h` configuration)
* [XPT2046_Touchscreen](https://github.com/PaulStoffregen/XPT2046_Touchscreen)
* [ArduinoJson](https://arduinojson.org/) (v6 or v7)
* [qrcode_espi](https://github.com/skelstar/arduino-qrcode-espi) (For rendering QRIS codes on the TFT)
* Standard ESP32 Core Libraries: `WiFi`, `WiFiMulti`, `HTTPClient`, `NetworkClientSecure`, `WebServer`, `EEPROM`

---

## ⚙️ Configuration & Setup

### 1. Midtrans Authentication
You must replace the placeholder Midtrans Server Key in the code with your actual Base64-encoded server key.

```cpp
// Replace <Midtrans-Auth> with your Base64 encoded Server Key
// Format: Basic [base64(server_key:)]
const char *midtransAuth = "Basic <Your-Base64-String>";
