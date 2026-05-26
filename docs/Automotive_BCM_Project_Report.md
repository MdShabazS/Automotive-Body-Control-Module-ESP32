# 📘 Project Report — Automotive Body Control Module (BCM) on ESP32

**Author:** Mohammed Shabaz S
**Domain:** Automotive Embedded Systems
**Platform:** Espressif ESP32 (Arduino framework)
**Status:** Working prototype

---

## 1. Abstract

This project presents the design and implementation of a miniature **Body Control Module (BCM)** — the ECU responsible for low-speed body electronics in modern vehicles — on an ESP32 microcontroller. The firmware models a real automotive control unit using a deterministic state machine, non-blocking event scheduling, and synchronized signaling, while interfacing with physical inputs (rotary ignition switch, toggle switches, brake button) and outputs (LEDs, passive buzzer, SSD1306 OLED dashboard).

---

## 2. Objectives

1. Replicate core BCM responsibilities: ignition state, brake light, indicators, hazards, audible feedback, and dashboard.
2. Apply automotive-grade firmware patterns: state machines, non-blocking timing, edge-triggered logging.
3. Build a fully wired, bench-testable prototype.
4. Lay a foundation for future CAN-bus and multi-ECU expansion.

---

## 3. System Architecture

### 3.1 Block Diagram

```
 ┌──────────────┐         ┌──────────────────────────┐         ┌────────────┐
 │  USER INPUTS │ ──────► │       ESP32 BCM CORE     │ ──────► │  ACTUATORS │
 │              │         │                          │         │            │
 │ • Rotary SW  │         │  ┌────────────────────┐  │         │ • LEDs     │
 │ • Brake PB   │         │  │ Ignition FSM       │  │         │ • Buzzer   │
 │ • Toggle L/R │         │  │ Debouncer          │  │         │ • OLED     │
 │              │         │  │ Blink Scheduler    │  │         │            │
 └──────────────┘         │  │ Output Multiplexer │  │         └────────────┘
                          │  └────────────────────┘  │
                          └──────────────────────────┘
                                     │
                                     ▼
                            ┌──────────────────┐
                            │   Serial Logger  │
                            │  (115200 baud)   │
                            └──────────────────┘
```

### 3.2 Software Layers

| Layer            | Responsibility                                             |
|------------------|------------------------------------------------------------|
| **HAL**          | `digitalRead`, `digitalWrite`, `tone`, `Wire`              |
| **Drivers**      | SSD1306 OLED (Adafruit_SSD1306), buzzer tone driver        |
| **State Machine**| Ignition FSM, indicator/hazard arbitration                 |
| **Scheduler**    | `millis()`-based blink + display refresh                   |
| **Application**  | Output multiplexer, OLED renderer, serial logger           |

---

## 4. Functional Specification

### 4.1 Ignition Behaviour

| State | Brake | Indicators | Hazards   | Dashboard      |
|-------|-------|------------|-----------|----------------|
| OFF   | ❌    | ❌         | ✅ active | "IGN: OFF"     |
| ACC   | ❌    | ❌         | ✅ active | "IGN: ACC"     |
| ON    | ✅    | ✅         | ✅ active | Full live data |

### 4.2 Indicator Logic

- Single toggle (Left **or** Right) + `IGN = ON` → that side blinks @ 1 Hz with buzzer chirp.
- Both toggles asserted → **Hazard mode**: both indicators blink synchronously, **regardless of ignition state** (mirrors real-vehicle behavior).
- Buzzer is driven via `tone(pin, 2000)` for an audible 2 kHz tone, gated by `blinkState`.

### 4.3 Brake Logic

- Brake LED follows debounced brake switch **only when ignition = ON**.
- Debounce window: 30 ms.

### 4.4 Display

- SSD1306 128×64 OLED over I²C @ 0x3C.
- Refreshed at 10 Hz to avoid loop-bottlenecking on the ~30 ms I²C transaction.
- "HAZARD" banner blinks in 2× font when hazard mode is active.

---

## 5. Implementation Highlights

### 5.1 Non-Blocking Scheduler

All time-based behavior uses `millis()` deltas — no `delay()` anywhere in the main loop. This guarantees the firmware can react to every input within one loop iteration (~3 ms typical).

```cpp
if (now - lastBlinkTs >= BLINK_INTERVAL_MS) {
  lastBlinkTs = now;
  blinkState = !blinkState;
}
```

### 5.2 Ignition State-Holding

When the rotary switch is between detents (no pin pulled LOW), the firmware **holds the previous valid state** rather than resetting — matching the mechanical reality of the switch.

### 5.3 Hazard Override

Hazards bypass the ignition gate, just like in production vehicles where the hazard circuit is wired direct to battery so it works even with the key removed.

### 5.4 Passive Buzzer

A passive buzzer is driven with `tone(pin, freq)` to generate an audible square wave. Earlier versions used `digitalWrite()` which only produced clicks — fixed in the current firmware.

---

## 6. Test Plan & Results

| #  | Test Case                                       | Expected                         | Result |
|----|-------------------------------------------------|----------------------------------|:------:|
| 1  | Boot                                            | OLED splash, `[INFO] BCM ready`  | ✅ Pass |
| 2  | Rotate ignition OFF→ACC→ON                      | `[IGN]` logs, OLED updates       | ✅ Pass |
| 3  | Brake press, IGN=OFF                            | Brake LED stays OFF              | ✅ Pass |
| 4  | Brake press, IGN=ON                             | Brake LED ON                     | ✅ Pass |
| 5  | Left toggle, IGN=ON                             | Left blinks @ 1 Hz + buzzer      | ✅ Pass |
| 6  | Right toggle, IGN=ON                            | Right blinks @ 1 Hz + buzzer     | ✅ Pass |
| 7  | Left toggle, IGN=OFF                            | LED stays OFF                    | ✅ Pass |
| 8  | Both toggles, IGN=OFF                           | Hazard mode active               | ✅ Pass |
| 9  | Both toggles, IGN=ON                            | Hazard mode active (overrides)   | ✅ Pass |
| 10 | OLED I²C disconnect                             | Boot prints fatal error          | ✅ Pass |
| 11 | Switch bounce on brake button                   | No false triggers                | ✅ Pass |

---

## 7. Performance

| Metric                       | Value           |
|------------------------------|-----------------|
| Loop iteration time (typ.)   | ~3 ms           |
| OLED refresh rate            | 10 Hz           |
| Indicator blink rate         | 1 Hz (500 ms)   |
| Buzzer frequency             | 2 kHz           |
| Brake debounce window        | 30 ms           |
| Flash usage (compiled)       | ~310 KB         |
| RAM usage                    | ~16 KB          |

---

## 8. Limitations

- No CAN bus — single-ECU prototype only.
- No persistent storage of vehicle state across boots.
- LED PWM is not used (binary on/off only).
- No automatic indicator cancellation logic (typically based on steering angle in real vehicles).

---

## 9. Future Work

1. **CAN bus integration** via MCP2515 transceiver.
2. **Speed-aware indicator auto-cancel.**
3. **Headlight automation** with LDR ambient light sensing.
4. **Diagnostic Trouble Codes (DTC)** module with non-volatile storage.
5. **OTA firmware updates** over Wi-Fi.
6. **Custom 4-layer PCB** for an automotive-grade prototype.

---

## 10. Conclusion

This project successfully demonstrates a real-time, non-blocking, automotive-grade firmware architecture on commodity hardware. It applies industry-standard embedded design patterns (FSMs, edge-triggered logging, debouncing, scheduled tasks) and produces a fully functional, bench-testable BCM prototype that is straightforward to extend toward production-grade automotive networks.

---

## 📚 References

- Espressif ESP32 Technical Reference Manual
- AUTOSAR Body Domain Specification (overview)
- Adafruit SSD1306 Library Documentation
- Bosch Automotive Handbook, 10th Edition
