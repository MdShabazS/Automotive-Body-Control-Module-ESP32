# 🔌 Wiring Guide — BCM ESP32

This document describes the complete hardware wiring for the Automotive Body Control Module prototype.

---

## 📋 Pin Mapping

| Signal              | ESP32 GPIO | Direction | Mode            | Notes                                     |
|---------------------|:----------:|:---------:|-----------------|-------------------------------------------|
| Ignition OFF        | 32         | Input     | `INPUT_PULLUP`  | Active LOW (rotary switch position 1)     |
| Ignition ACC        | 33         | Input     | `INPUT_PULLUP`  | Active LOW (rotary switch position 2)     |
| Ignition ON         | 25         | Input     | `INPUT_PULLUP`  | Active LOW (rotary switch position 3)     |
| Brake Switch        | 14         | Input     | `INPUT_PULLUP`  | Active LOW (push button)                  |
| Left Indicator SW   | 26         | Input     | `INPUT_PULLUP`  | Active LOW (toggle)                       |
| Right Indicator SW  | 27         | Input     | `INPUT_PULLUP`  | Active LOW (toggle)                       |
| Brake LED           | 17         | Output    | Digital         | Series 220 Ω resistor                     |
| Left Indicator LED  | 4          | Output    | Digital         | Series 220 Ω resistor                     |
| Right Indicator LED | 16         | Output    | Digital         | Series 220 Ω resistor                     |
| Passive Buzzer      | 13         | Output    | `tone()` PWM    | Driven at 2 kHz when active               |
| OLED SDA            | 21         | I²C       | -               | SSD1306 default                           |
| OLED SCL            | 22         | I²C       | -               | SSD1306 default                           |
| OLED VCC / GND      | 3V3 / GND  | Power     | -               | OLED is 3.3 V tolerant                    |

> ⚠️ On ESP32-WROVER modules, GPIOs **16** and **17** are reserved for PSRAM. Use the WROOM-32 variant or remap those pins (e.g., to 18 and 19) if you switch boards.

---

## 🧩 Schematic (ASCII)

```
                            ┌───────────────────────┐
                       3V3 ─┤ VCC                   │
                       GND ─┤ GND                   │
                            │                       │
   ┌─[Rotary SW]───┬────────┤ 32  IGN_OFF           │
   │               ├────────┤ 33  IGN_ACC           │
   │               └────────┤ 25  IGN_ON            │
   GND                      │                       │
                            │                       │
   ┌─[Brake PB]─────────────┤ 14  BRAKE_SW          │
   GND                      │                       │
                            │                       │
   ┌─[Toggle L]─────────────┤ 26  LEFT_SW           │
   GND                      │                       │
   ┌─[Toggle R]─────────────┤ 27  RIGHT_SW          │
   GND                      │                       │
                            │           ESP32       │
                            │                       │
                            │ 17  BRAKE_LED ────[220Ω]──▷|── GND
                            │  4  LEFT_LED  ────[220Ω]──▷|── GND
                            │ 16  RIGHT_LED ────[220Ω]──▷|── GND
                            │ 13  BUZZER    ──────[Passive Buzzer]── GND
                            │                       │
                            │ 21  SDA  ─────────────┐
                            │ 22  SCL  ────────────┐│
                            └───────────────────────┘│
                                                    ││
                                          ┌─────────┴┴───────┐
                                          │   SSD1306 OLED   │
                                          │  VCC GND SDA SCL │
                                          └──────────────────┘
```

---

## 🔋 Power

- ESP32 powered via USB (5 V → onboard 3.3 V LDO).
- All peripherals run on **3.3 V** from the ESP32 (do NOT connect 5 V to the OLED data lines).
- LEDs: 220 Ω series resistors limit current to ~10 mA per channel.

---

## ✅ Bring-up Checklist

1. Verify continuity from each switch to GND when actuated.
2. Power the board via USB; confirm the OLED prints `BCM ESP32 Ready`.
3. Open Serial Monitor at **115200 baud**; rotate ignition switch and watch `[IGN]` log lines.
4. Press brake — `BRAKE_LED` should light only when ignition is `ON`.
5. Toggle left switch — `LEFT_LED` blinks @ 1 Hz (500 ms ON/OFF) with audible buzzer chirp.
6. Toggle both switches — both LEDs blink in sync, OLED shows large `HAZARD` text. Hazards must work even with ignition `OFF`.

---

## 🧰 Troubleshooting

| Symptom                              | Likely Cause                                  |
|--------------------------------------|-----------------------------------------------|
| OLED stays blank                     | I²C address mismatch (try 0x3D), bad wiring   |
| LED always ON                        | Pin shorted to GND, or output set to wrong level |
| Indicators don't blink               | Not in `IGN_STATE_ON` (unless hazard mode)    |
| Buzzer only clicks                   | Wrong buzzer type — must be **passive**       |
| Brake LED doesn't react              | Brake switch wired to 3V3 instead of GND      |
| Boot loops on power-up               | Strapping pin pulled wrong (check GPIO 12/15) |
