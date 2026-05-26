<div align="center">

# 🚗 Automotive Body Control Module (BCM)

### Real-Time Vehicle Body Electronics Firmware on ESP32

A production-style automotive ECU firmware that simulates ignition, brake, indicators, hazards, and an OLED dashboard — driven by a non-blocking state machine on the ESP32.

[![Platform](https://img.shields.io/badge/Platform-ESP32-E7352C?logo=espressif&logoColor=white)](https://www.espressif.com/en/products/socs/esp32)
[![Framework](https://img.shields.io/badge/Framework-Arduino-00979D?logo=arduino&logoColor=white)](https://www.arduino.cc/)
[![Language](https://img.shields.io/badge/Language-C%2B%2B-00599C?logo=cplusplus&logoColor=white)](https://isocpp.org/)
[![Display](https://img.shields.io/badge/Display-SSD1306-000000?logo=oled&logoColor=white)]()
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Status](https://img.shields.io/badge/status-working%20prototype-brightgreen)]()

</div>

---

## 📌 Overview

**BCM-ESP32** is a compact, real-time **Body Control Module** firmware modeled on production-grade automotive ECU patterns. It implements a 3-state ignition machine, brake light control, turn indicators, hazard mode, synchronized audible feedback, and a live OLED instrument cluster — all driven by a non-blocking event loop.

Aimed at automotive embedded learners, hobbyist ECU tinkerers, and anyone who wants a clean reference for state-machine firmware on the ESP32 / Arduino framework.

## ✨ Highlights

- **Deterministic ignition FSM** — `OFF / ACC / ON` with state-holding between rotary detents, mirroring real switch behavior.
- **Hazard override** — both indicators blink synchronously, *even with ignition OFF*, just like a production vehicle.
- **Non-blocking scheduler** — pure `millis()`-based timing, zero `delay()` in the main loop.
- **Passive buzzer driver** — `tone()`-based 2 kHz square wave, gated by blink state.
- **Throttled OLED refresh** — 10 Hz redraw avoids bottlenecking on the ~30 ms I²C transaction.
- **Brake debouncing** — 30 ms window eliminates push-button chatter.
- **Edge-triggered serial logging** — every signal logs only on state change, keeping the bus clean.
- **Clean GPIO abstraction** — every pin defined once at the top, easy to remap.

## 🧠 Architecture

```
┌──────────────┐         ┌──────────────────────────┐         ┌──────────────┐
│  USER INPUTS │ ──────▶ │       ESP32 BCM CORE     │ ──────▶ │   ACTUATORS  │
│              │         │                          │         │              │
│  Rotary SW   │         │   Ignition FSM           │         │  Brake LED   │
│  Brake PB    │         │   Debouncer              │         │  L / R LEDs  │
│  Toggle L/R  │         │   Blink Scheduler        │         │  Buzzer      │
│              │         │   Output Multiplexer     │         │  OLED (I²C)  │
└──────────────┘         └────────────┬─────────────┘         └──────────────┘
                                      │
                                      ▼
                              ┌────────────────┐
                              │ Serial Logger  │
                              │  115200 baud   │
                              └────────────────┘
```

## 📁 Repository Layout

```
Automotive-Body-Control-Module-ESP32/
├── code/
│   └── bcm_esp32.ino              # Main firmware
├── docs/
│   ├── wiring.md                  # Pin map, schematic, bring-up checklist
│   └── Automotive_BCM_Project_Report.md
├── images/
│   ├── hardware_setup.jpg
│   └── indicator_on.jpg
├── LICENSE
├── .gitignore
└── README.md
```

## 🚀 Quick Start

### 1. Prerequisites

| Dependency | Purpose | Install |
| ---------- | ------- | ------- |
| Arduino IDE ≥ 2.0 | Build & flash | <https://www.arduino.cc/en/software> |
| ESP32 board package | Toolchain | Boards Manager → "esp32 by Espressif Systems" |
| `Adafruit GFX Library` | Display primitives | Library Manager |
| `Adafruit SSD1306` | OLED driver | Library Manager |

### 2. Hardware

| Component | Qty | Notes |
| --------- | :-: | ----- |
| ESP32 Dev Board (WROOM-32) | 1 | Tested on DevKit V1 |
| SSD1306 OLED 128×64 (I²C) | 1 | Address `0x3C` |
| 3-position rotary switch | 1 | Active-LOW to GND |
| SPST toggle switch | 2 | Indicators L / R |
| Tactile push button | 1 | Brake |
| 5 mm LED + 220 Ω resistor | 3 | Brake / L / R |
| Passive buzzer | 1 | Driven by `tone()` |

### 3. Build & Flash

```bash
git clone https://github.com/MdShabazS/Automotive-Body-Control-Module-ESP32.git
```

1. Open `code/bcm_esp32.ino` in Arduino IDE.
2. Select **Tools → Board → ESP32 Dev Module**.
3. Select the correct **Port**.
4. Click **Upload**.
5. Open Serial Monitor at **115200 baud**.

## 🎮 Usage

1. Power the board via USB. The OLED briefly shows `BCM ESP32 Ready`.
2. Rotate the ignition switch through `OFF → ACC → ON`. Each transition emits an `[IGN]` log line.
3. Press the brake button — the brake LED lights only when ignition is `ON`.
4. Toggle either indicator switch — that side blinks at 1 Hz with an audible 2 kHz buzzer chirp.
5. Toggle **both** indicator switches — hazard mode engages with synchronized blinking and a large `HAZARD` banner on the OLED. Hazards remain active even when ignition is `OFF`.

## 🔌 Pin Map

| Signal | GPIO | Mode |
| ------ | :--: | ---- |
| Ignition OFF | 32 | `INPUT_PULLUP` |
| Ignition ACC | 33 | `INPUT_PULLUP` |
| Ignition ON  | 25 | `INPUT_PULLUP` |
| Brake switch | 14 | `INPUT_PULLUP` |
| Left switch  | 26 | `INPUT_PULLUP` |
| Right switch | 27 | `INPUT_PULLUP` |
| Brake LED    | 17 | Output |
| Left LED     |  4 | Output |
| Right LED    | 16 | Output |
| Buzzer       | 13 | `tone()` PWM |
| OLED SDA / SCL | 21 / 22 | I²C |

> On ESP32-WROVER modules, GPIOs 16 & 17 are reserved for PSRAM. Use a WROOM-32 board or remap. Full schematic in [`docs/wiring.md`](docs/wiring.md).

## ⚙️ Configuration

All tunables live at the top of `code/bcm_esp32.ino`:

```cpp
const unsigned long BLINK_INTERVAL_MS  = 500;   // Indicator blink period
const unsigned long DISPLAY_REFRESH_MS = 100;   // OLED redraw period
const unsigned long DEBOUNCE_MS        = 30;    // Brake debounce window
#define BUZZER_FREQ                    2000     // Buzzer tone (Hz)
#define OLED_ADDR                      0x3C     // SSD1306 I²C address
```

## 🔄 Operating Logic

| Ignition | Brake | Indicators | Hazards | Dashboard |
| -------- | :---: | :--------: | :-----: | --------- |
| OFF      |   ✗   |     ✗      |    ✓    | `IGN: OFF` |
| ACC      |   ✗   |     ✗      |    ✓    | `IGN: ACC` |
| ON       |   ✓   |     ✓      |    ✓    | Full live status |

## 📷 Prototype Images

### Hardware Setup
![Hardware Setup](images/hardware_setup.jpg)

### Indicator Active
![Indicator](images/indicator_on.jpg)

## 🗺️ Roadmap

- [ ] CAN-bus integration via MCP2515 (multi-ECU communication)
- [ ] Speed-aware indicator auto-cancel
- [ ] Automatic headlight control (LDR-based)
- [ ] Diagnostic Trouble Codes (DTC) module with non-volatile storage
- [ ] OTA firmware updates over Wi-Fi
- [ ] Custom 4-layer PCB for an automotive-grade prototype

## 🤝 Contributing

Issues and pull requests are welcome. For larger changes, please open an issue first to discuss what you would like to change.

```bash
git checkout -b feature/my-change
# ... commits ...
git push origin feature/my-change
```

## 📜 License

Released under the [MIT License](LICENSE).

## 👨‍💻 Author

<div align="center">

**Mohammed Shabaz S**
Electronics & Communication Engineering · Automotive Embedded Systems

[![GitHub](https://img.shields.io/badge/GitHub-MdShabazS-181717?logo=github)](https://github.com/MdShabazS)
[![LinkedIn](https://img.shields.io/badge/LinkedIn-Connect-0A66C2?logo=linkedin)](https://www.linkedin.com/)

⭐ *If you found this project useful, please consider giving it a star — it helps others discover it!*

</div>
