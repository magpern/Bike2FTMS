# ANT+ to BLE FTMS Bridge
[![Build](https://github.com/magpern/Bike2FTMS/actions/workflows/build.yml/badge.svg)](https://github.com/magpern/Bike2FTMS/actions/workflows/build.yml)
[![Release](https://img.shields.io/github/v/release/magpern/Bike2FTMS?label=Release)](https://github.com/magpern/Bike2FTMS/releases)
[![License](https://img.shields.io/badge/license-MIT--NC-blue)](LICENSE)
![Platform](https://img.shields.io/badge/platform-nRF52840-blue)
![SDK](https://img.shields.io/badge/SDK-nRF5%20SDK%2017.1.0-blue)
![SoftDevice](https://img.shields.io/badge/SoftDevice-S340%20v7.0.1-orange)
[![Lines of Code](https://tokei.rs/b1/github/magpern/Bike2FTMS?category=code)](https://github.com/magpern/Bike2FTMS)
![Firmware Size](https://img.shields.io/badge/firmware-compiled-lightgrey)
![Status](https://img.shields.io/badge/project-🔥%20Ready%20to%20Test-success)

## **Project Overview**
This project implements an **ANT+ to BLE bridge** using an **nRF52840** microcontroller. The bridge listens for **ANT+ spin bike data** (device type 11), processes it, and transmits it using the **BLE FTMS (Fitness Machine Service)** protocol. This allows BLE-enabled fitness applications to receive real-time cycling data.

The firmware is optimized for **low-power operation**, implementing sleep modes and event-driven ANT+ scanning to ensure efficiency.

---

## **Features**
### ✅ **Implemented Features**
- **BLE FTMS Support**
  - Implements FTMS characteristics for cycling power, cadence, and training status.
  - Supports **Indoor Bike Data (0x2AD2)**, **Training Status (0x2AD3)**, and **Fitness Machine Status (0x2ADA)**.
- **ANT+ Bicycle Power Profile (Device Type 11)**
  - Listens for ANT+ power meter broadcasts.
  - Parses power, cadence, and additional data.
- **Device Configuration via BLE**
  - Allows setting the **ANT+ Device ID** and **BLE name** dynamically.
  - Settings persist across reboots using **UICR flash storage**.
- **NFC Support for Quick Setup**
  - Scans NFC tags to configure **ANT+ Device ID and BLE name**.
    (Implemented, but not active)
- **Power Management Features**
  - BLE advertising starts **only when ANT+ data is detected**.
  - Automatically **enters deep sleep** when no ANT+ devices are broadcasting.
  - Implements **wake-up on ANT+ signal detection**.

---

## **System Workflow**
1. The **nRF52840 wakes up** and starts scanning for ANT+ broadcasts.
2. If a valid **ANT+ spin bike** is found:
   - BLE advertising starts.
   - FTMS notifications begin transmitting cycling data.
3. If no ANT+ data is detected for **10 seconds**:
   - BLE advertising stops.
   - The device enters **deep sleep**.
4. The device **periodically wakes up** to check for new ANT+ broadcasts.

---

## **Setup & Usage**
[![Download Firmware](https://img.shields.io/badge/download-satsbike_dfu.zip-blue?logo=github)](https://github.com/magpern/Bike2FTMS/releases/latest/download/satsbike_dfu.zip)
### **1️⃣ Required Development Tools**
- **nRF5 SDK v17.1.0**
- **SoftDevice S340 v7.2.0**
- **GNU ARM Toolchain** (for compilation)
- **nrfjprog** (for flashing/debugging)
- **Segger J-Link** (for debugging)

### **2️⃣ Project Directory Structure**
```
D:/nRF5_SDK/projects/bleant_multi
│-- pca10056/s340/armgcc   # Build and Makefiles
│-- src/                   # Source code
│-- include/               # Header files
│-- Readme.md              # Project documentation
```

### **3️⃣ Build & Flashing Commands**
```sh
cd D:/nRF5_SDK/projects/bleant_multi/pca10056/s340/armgcc
make
make flash
```

### **4️⃣ Debugging in VS Code**
- Ensure `nrfjprog` and `J-Link` are installed.
- Use `make debug` for live debugging.

---

## **To-Do List & Future Improvements**
### **1️⃣ Final Testing & Optimization**
- [ ] Verify ANT+ scanning reliability with multiple bikes.
- [ ] Test FTMS notifications with fitness apps.
- [ ] Measure power consumption in different states.

### **2️⃣ Multi-Bike & Multi-Client Support**
- [ ] Enable support for **multiple ANT+ bikes** (future scope).
- [ ] Allow up to **15 BLE clients** to connect (future scope).

### **3️⃣ Additional Enhancements**
- [ ] Implement a **custom GATT service** for BLE client configuration.
- [ ] Improve logging and debugging capabilities.

---

## **Next Steps**
- 🚀 **Prepare for final acceptance testing.**
- 🔬 **Measure power consumption and optimize sleep modes. **
- 📡 **Test real-world BLE & ANT+ performance.**

---

✅ **This project is near completion, ready for acceptance testing!**
