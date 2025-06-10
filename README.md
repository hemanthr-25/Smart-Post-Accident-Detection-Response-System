#🚨 Smart Post-Accident Detection & Response System(Auto SOS)


This project was built as part of our 5th hackathon — and it's the one that helped us **secure our 4th win**.  
The goal: to **automate emergency response after vehicle crashes**, especially in remote or low-traffic areas, using IoT and real-time decision logic.

#🧠 Idea

Every second matters after a crash. But what if:
- The victim is unconscious?
- No one is around to call for help?
- Emergency services are delayed because of traffic?

We built a system that **detects crashes, confirms them, alerts family/police, and clears traffic signals** — all without human intervention.

---

## 📦 System Overview

We divided our system into **4 subsystems**:

### 🚗 Vehicle Subsystem
- Detects crash .
- Displays a **15-second timer** to cancel false triggers.
- If no response:
  - Captures images from **dashcam**.
  - Fetches **GPS location**.
  - Sends alerts to **family members**.

### 🆘 SOS Subsystem
- Sends emergency notifications to:
  - Family
  - Police
  - Ambulance
- Includes a **2-minute window** for ambulance confirmation.
- No response? System automatically escalates.

### 🚑 Ambulance Subsystem
- Receives dispatch notification.
- Communicates with **traffic signal controllers** via **NRF module**.

### 🚦 Traffic Signal Subsystem
- Receives signal from ambulance.
- Automatically sets green path along ambulance route.
- Restores normal signal after passage.

---

## 📸 Demo

Check out the [YouTube video]([https://youtube.com/7volts_channel_link_here](https://youtu.be/k_r8A9lgWos)) for full demo, architecture, and team story.

---

## 🛠️ Tech Stack

- **ESP32** (Vehicle + Ambulance logic)
- **NRF24L01** (Signal communication)
- **Node-RED Dashboard** (Live UI + timers)
- **MQTT (HiveMQ)** for message passing
- **Neo 6M GPS Module**
- **esp32 Camera Modules**


