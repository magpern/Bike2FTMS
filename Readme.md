# ANT+ to BLE FTMS Bridge

## **Project Overview**
This project aims to create an **energy-efficient ANT+ to BLE bridge** using an **nRF52840** device. The goal is to receive data from **ANT+ spin bikes** (device type 11), process it, and transmit it via **BLE FTMS (Fitness Machine Service)**.

The bridge must be optimized for **low-power operation**, ensuring that ANT+ and BLE **only run when needed**. This involves implementing sleep modes and efficient power management.

---

## **Current Status**
### ✅ **Existing Features**
- **BLE FTMS Service**: Implements FTMS with characteristics for cycling data.
- **ANT+ Bicycle Power Profile**: Receives data from ANT+ spin bikes.
- **Device Configuration via BLE**: Allows setting ANT+ Device ID and BLE name via BLE.
- **Basic Power Management**: Currently, both BLE and ANT+ are always on.

### ❌ **Issues & Improvements Needed**
- **High Power Consumption**: BLE and ANT+ are always enabled.
- **No Sleep Mode**: The device does not enter low-power mode when idle.
- **No ANT+ Polling Strategy**: The device should wake periodically to check for ANT+ signals instead of always listening.

---

## **To-Do List**
### **1️⃣ Implement Sleep & Wake Strategies**
- [ ] **Setup Mode**: Keep BLE active when no ANT+ device is found for configuration.
- [ ] **Sleep if No ANT+ Device Found**:
  - If the configured ANT+ device is **not broadcasting**, enter **deep sleep** for a set time.
  - Wake up, scan for ANT+ messages, then return to sleep if none are found.
- [ ] **Wake on ANT+ Detection**:
  - Enable BLE **only** when valid ANT+ messages are received.

### **2️⃣ Conditional BLE Activation & Timeout**
- [ ] **Disable BLE Until ANT+ Detected**:
  - BLE should not start advertising until an ANT+ device is active.
- [ ] **Turn Off BLE When No ANT+ Messages**:
  - If no ANT+ messages arrive for **X minutes**, BLE should stop and the device should enter sleep mode.

### **3️⃣ Improve Configuration & Persistence**
- [ ] **Save Device ID & BLE Name**:
  - Ensure ANT+ Device ID and BLE name persist across reboots.
- [ ] **Enable NFC for Quick Setup** (Future Improvement):
  - Use NFC to set ANT+ Device ID and BLE name.

### **4️⃣ Test & Optimize Power Consumption**
- [ ] **Measure Power Usage**:
  - Test power consumption in different states (Active, Sleep, Wake-up).
- [ ] **Optimize Sleep & Wake Timers**:
  - Find the best trade-off between power savings and responsiveness.

---

## **Development Setup**
### **Required Tools**
- **nRF5 SDK v17.1.0**
- **SoftDevice S140 v7.2.0**
- **GNU ARM Toolchain** (for compilation)
- **nrfjprog** (for flashing/debugging)
- **Segger J-Link** (for debugging)

### **Project Structure**
```
D:/nRF5_SDK/projects/bleant_multi
│-- pca10056/s340/armgcc   # Build and Makefiles
│-- src/                   # Source code
│-- include/               # Header files
```

### **Build & Flashing Commands**
```sh
cd D:/nRF5_SDK/projects/bleant_multi/pca10056/s340/armgcc
make
make flash
```

### **Debugging in VS Code**
- Ensure `nrfjprog` and `J-Link` are installed.
- Use `make debug` for live debugging.

---

## **Next Steps**
- [ ] **Confirm Implementation Plan** before coding.
- [ ] **Start with Sleep & Wake Strategies**.
- [ ] **Test & Optimize Power Management**.

🚀 **Let’s make this the most efficient ANT+ to BLE bridge possible!**

