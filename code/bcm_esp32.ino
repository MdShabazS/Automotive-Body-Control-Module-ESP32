#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ---------- PIN DEFINITIONS ----------

// Ignition
#define IGN_OFF 32
#define IGN_ACC 33
#define IGN_ON 25

// Brake
#define BRAKE_SW 14
#define BRAKE_LED 17

// Indicators
#define LEFT_SW 26
#define RIGHT_SW 27
#define LEFT_LED 4
#define RIGHT_LED 16

// Buzzer
#define BUZZER 13

// ---------- VARIABLES ----------

enum IgnitionState {OFF, ACC, ON};
IgnitionState ignition = OFF;
IgnitionState previousIgnition = OFF;

bool brakeState = false;
bool previousBrakeState = false;

bool leftActive = false;
bool rightActive = false;

unsigned long previousMillis = 0;
const long blinkInterval = 500;
bool blinkState = false;

void setup() {
  Serial.begin(115200);
  Wire.begin(21,22);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  // Ignition
  pinMode(IGN_OFF, INPUT_PULLUP);
  pinMode(IGN_ACC, INPUT_PULLUP);
  pinMode(IGN_ON, INPUT_PULLUP);

  // Brake
  pinMode(BRAKE_SW, INPUT_PULLUP);
  pinMode(BRAKE_LED, OUTPUT);

  // Indicators
  pinMode(LEFT_SW, INPUT_PULLUP);
  pinMode(RIGHT_SW, INPUT_PULLUP);
  pinMode(LEFT_LED, OUTPUT);
  pinMode(RIGHT_LED, OUTPUT);

  // Buzzer
  pinMode(BUZZER, OUTPUT);

  Serial.println("BCM Phase 4 Starting...");
}

void loop() {

  // -------- IGNITION DETECTION --------
  if (digitalRead(IGN_OFF) == LOW) ignition = OFF;
  else if (digitalRead(IGN_ACC) == LOW) ignition = ACC;
  else if (digitalRead(IGN_ON) == LOW) ignition = ON;

  // -------- BRAKE --------
  brakeState = (digitalRead(BRAKE_SW) == LOW && ignition == ON);
  digitalWrite(BRAKE_LED, brakeState);

  // -------- INDICATOR SWITCHES --------
  leftActive = (digitalRead(LEFT_SW) == LOW && ignition == ON);
  rightActive = (digitalRead(RIGHT_SW) == LOW && ignition == ON);

  // -------- BLINK TIMER --------
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= blinkInterval) {
    previousMillis = currentMillis;
    blinkState = !blinkState;
  }

  // -------- INDICATOR OUTPUT --------
  digitalWrite(LEFT_LED, leftActive ? blinkState : LOW);
  digitalWrite(RIGHT_LED, rightActive ? blinkState : LOW);

  // -------- BUZZER --------
  bool indicatorActive = leftActive || rightActive;
  digitalWrite(BUZZER, indicatorActive ? blinkState : LOW);

  // -------- SERIAL LOGGING --------
  if (ignition != previousIgnition) {
    Serial.print("Ignition: ");
    if (ignition == OFF) Serial.println("OFF");
    else if (ignition == ACC) Serial.println("ACC");
    else Serial.println("ON");
    previousIgnition = ignition;
  }

  if (brakeState != previousBrakeState) {
    Serial.print("Brake: ");
    Serial.println(brakeState ? "ON" : "OFF");
    previousBrakeState = brakeState;
  }

  // -------- OLED DASHBOARD --------
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);

  display.print("Ignition: ");
  if (ignition == OFF) display.println("OFF");
  else if (ignition == ACC) display.println("ACC");
  else display.println("ON");

  display.print("Brake: ");
  display.println(brakeState ? "ON" : "OFF");

  display.print("Left: ");
  display.println(leftActive ? "ON" : "OFF");

  display.print("Right: ");
  display.println(rightActive ? "ON" : "OFF");

  display.display();
}