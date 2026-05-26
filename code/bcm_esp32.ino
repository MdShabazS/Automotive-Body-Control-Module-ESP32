/*
 * ============================================================================
 *  Automotive Body Control Module (BCM) - ESP32
 * ============================================================================
 *  Author : Mohammed Shabaz S
 *  License: MIT
 *
 *  Features:
 *    - 4-state ignition machine (OFF / ACC / ON / INVALID)
 *    - Brake light control (gated by ignition = ON)
 *    - Left/Right turn indicators with synchronized blink
 *    - Hazard mode (both indicators) - works even when ignition is OFF
 *    - Passive buzzer driven via tone() at 2 kHz, sync'd with blink
 *    - SSD1306 OLED dashboard (throttled redraw @ 10 Hz)
 *    - Brake-input debouncing
 *    - Edge-triggered serial logging
 * ============================================================================
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------- DISPLAY ----------
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_ADDR     0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ---------- PIN DEFINITIONS ----------
// Ignition (rotary switch, active-LOW)
#define IGN_OFF     32
#define IGN_ACC     33
#define IGN_ON      25

// Brake
#define BRAKE_SW    14
#define BRAKE_LED   17

// Indicators
#define LEFT_SW     26
#define RIGHT_SW    27
#define LEFT_LED     4
#define RIGHT_LED   16

// Buzzer (passive)
#define BUZZER      13
#define BUZZER_FREQ 2000   // Hz

// ---------- TIMING ----------
const unsigned long BLINK_INTERVAL_MS  = 500;
const unsigned long DISPLAY_REFRESH_MS = 100;
const unsigned long DEBOUNCE_MS        = 30;

// ---------- STATE TYPES ----------
enum IgnitionState { IGN_STATE_OFF, IGN_STATE_ACC, IGN_STATE_ON };

// ---------- STATE VARIABLES ----------
IgnitionState ignition         = IGN_STATE_OFF;
IgnitionState previousIgnition = IGN_STATE_OFF;

bool brakeRaw          = false;
bool brakeStable       = false;
bool previousBrake     = false;
unsigned long brakeChangeTs = 0;

bool leftActive   = false;
bool rightActive  = false;
bool hazardActive = false;
bool prevLeft = false, prevRight = false, prevHazard = false;

unsigned long lastBlinkTs   = 0;
unsigned long lastDisplayTs = 0;
bool blinkState = false;

// ============================================================================
//  HELPERS
// ============================================================================
const char* ignitionToStr(IgnitionState s) {
  switch (s) {
    case IGN_STATE_OFF: return "OFF";
    case IGN_STATE_ACC: return "ACC";
    case IGN_STATE_ON:  return "ON";
  }
  return "?";
}

IgnitionState readIgnition() {
  // Active-LOW rotary switch. If none asserted -> hold previous (between detents).
  if (digitalRead(IGN_OFF) == LOW) return IGN_STATE_OFF;
  if (digitalRead(IGN_ACC) == LOW) return IGN_STATE_ACC;
  if (digitalRead(IGN_ON)  == LOW) return IGN_STATE_ON;
  return ignition; // hold last valid state
}

void updateBrakeDebounced() {
  bool reading = (digitalRead(BRAKE_SW) == LOW);
  if (reading != brakeRaw) {
    brakeRaw = reading;
    brakeChangeTs = millis();
  }
  if (millis() - brakeChangeTs >= DEBOUNCE_MS) {
    brakeStable = brakeRaw;
  }
}

void setBuzzer(bool on) {
  static bool buzzerOn = false;
  if (on && !buzzerOn) {
    tone(BUZZER, BUZZER_FREQ);
    buzzerOn = true;
  } else if (!on && buzzerOn) {
    noTone(BUZZER);
    digitalWrite(BUZZER, LOW);
    buzzerOn = false;
  }
}

// ============================================================================
//  DISPLAY
// ============================================================================
void renderDashboard() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Header
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("BCM  |  IGN: ");
  display.println(ignitionToStr(ignition));
  display.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);

  // Body
  display.setCursor(0, 16);
  display.print("Brake : ");
  display.println(brakeStable ? "ON" : "OFF");

  display.print("Left  : ");
  display.println(leftActive ? "BLINK" : "OFF");

  display.print("Right : ");
  display.println(rightActive ? "BLINK" : "OFF");

  if (hazardActive) {
    display.setTextSize(2);
    display.setCursor(8, 46);
    display.print(blinkState ? "HAZARD" : "      ");
  }

  display.display();
}

// ============================================================================
//  SETUP
// ============================================================================
void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.println(F("\n=== BCM ESP32 Boot ==="));

  // Inputs
  pinMode(IGN_OFF,  INPUT_PULLUP);
  pinMode(IGN_ACC,  INPUT_PULLUP);
  pinMode(IGN_ON,   INPUT_PULLUP);
  pinMode(BRAKE_SW, INPUT_PULLUP);
  pinMode(LEFT_SW,  INPUT_PULLUP);
  pinMode(RIGHT_SW, INPUT_PULLUP);

  // Outputs
  pinMode(BRAKE_LED, OUTPUT);
  pinMode(LEFT_LED,  OUTPUT);
  pinMode(RIGHT_LED, OUTPUT);
  pinMode(BUZZER,    OUTPUT);
  digitalWrite(BRAKE_LED, LOW);
  digitalWrite(LEFT_LED,  LOW);
  digitalWrite(RIGHT_LED, LOW);
  digitalWrite(BUZZER,    LOW);

  // I2C + OLED
  Wire.begin(21, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("[FATAL] SSD1306 init failed"));
    while (true) { delay(1000); }
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(10, 24);
  display.println(F("BCM ESP32 Ready"));
  display.display();
  delay(800);

  Serial.println(F("[INFO] BCM ready"));
}

// ============================================================================
//  MAIN LOOP
// ============================================================================
void loop() {
  unsigned long now = millis();

  // 1. Read ignition
  ignition = readIgnition();

  // 2. Debounced brake
  updateBrakeDebounced();

  // 3. Read indicator switches
  bool leftSw  = (digitalRead(LEFT_SW)  == LOW);
  bool rightSw = (digitalRead(RIGHT_SW) == LOW);

  // 4. Determine modes
  hazardActive = (leftSw && rightSw);                          // hazards work in any ignition state
  bool turnAllowed = (ignition == IGN_STATE_ON) && !hazardActive;
  leftActive   = hazardActive ? true : (turnAllowed && leftSw);
  rightActive  = hazardActive ? true : (turnAllowed && rightSw);

  // 5. Brake (only when ignition ON)
  bool brakeOut = brakeStable && (ignition == IGN_STATE_ON);
  digitalWrite(BRAKE_LED, brakeOut);

  // 6. Blink scheduler
  if (now - lastBlinkTs >= BLINK_INTERVAL_MS) {
    lastBlinkTs = now;
    blinkState = !blinkState;
  }

  // 7. Drive indicator outputs
  digitalWrite(LEFT_LED,  (leftActive  ? blinkState : LOW));
  digitalWrite(RIGHT_LED, (rightActive ? blinkState : LOW));

  // 8. Buzzer follows blink whenever any indicator is active
  bool indicatorOn = (leftActive || rightActive);
  setBuzzer(indicatorOn && blinkState);

  // 9. Edge-triggered serial logging
  if (ignition != previousIgnition) {
    Serial.print(F("[IGN] "));
    Serial.println(ignitionToStr(ignition));
    previousIgnition = ignition;
  }
  if (brakeOut != previousBrake) {
    Serial.print(F("[BRAKE] "));
    Serial.println(brakeOut ? "ON" : "OFF");
    previousBrake = brakeOut;
  }
  if (hazardActive != prevHazard) {
    Serial.print(F("[HAZARD] "));
    Serial.println(hazardActive ? "ON" : "OFF");
    prevHazard = hazardActive;
  }
  if (leftActive != prevLeft && !hazardActive) {
    Serial.print(F("[LEFT] "));
    Serial.println(leftActive ? "ON" : "OFF");
    prevLeft = leftActive;
  }
  if (rightActive != prevRight && !hazardActive) {
    Serial.print(F("[RIGHT] "));
    Serial.println(rightActive ? "ON" : "OFF");
    prevRight = rightActive;
  }

  // 10. Throttled OLED refresh
  if (now - lastDisplayTs >= DISPLAY_REFRESH_MS) {
    lastDisplayTs = now;
    renderDashboard();
  }
}
