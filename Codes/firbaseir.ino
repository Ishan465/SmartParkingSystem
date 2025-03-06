#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include "time.h"

// WiFi credentials
const char* ssid = "qwerty";
const char* password = "plmnbvcxz";

// Firebase credentials
#define FIREBASE_HOST //key
#define FIREBASE_AUTH //key

// NTP Server details
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -18000;    // EST timezone offset
const int   daylightOffset_sec = 3600; // Daylight saving adjustment

FirebaseData fbData;
FirebaseAuth auth;
FirebaseConfig config;

// Hardware definitions
Servo gate;
const int servoPin = 2;
const int ir_gate_in = 15;
const int ir_gate_out = 4;
const int ir_car1 = 13;
const int ir_car2 = 12;
const int ir_car3 = 14;
const int ir_car4 = 27;

int S1 = 0, S2 = 0, S3 = 0, S4 = 0;
int gatePosition = 0;
unsigned long lastGateTrigger = 0;
const unsigned long debounceDelay = 500;

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(115200);
  
  // Initialize LCD
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Smart Parking");

  // Initialize pins
  pinMode(ir_gate_in, INPUT_PULLUP);
  pinMode(ir_gate_out, INPUT_PULLUP);
  pinMode(ir_car1, INPUT_PULLUP);
  pinMode(ir_car2, INPUT_PULLUP);
  pinMode(ir_car3, INPUT_PULLUP);
  pinMode(ir_car4, INPUT_PULLUP);

  gate.attach(servoPin);
  gate.write(0);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  // Initialize NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();

  // Initialize Firebase
  config.database_url = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);

  lcd.setCursor(0, 1);
  lcd.print("Emp: 4 Occ: 0 ");
}

String getCurrentTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Failed to obtain time";
  }
  char timeString[25];
  strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(timeString);
}

void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%Y-%m-%d %H:%M:%S");
}

void loop() {
  // Read sensors
  S1 = !digitalRead(ir_car1);
  S2 = !digitalRead(ir_car2);
  S3 = !digitalRead(ir_car3);
  S4 = !digitalRead(ir_car4);

  int occupied = S1 + S2 + S3 + S4;
  int empty = 4 - occupied;
  unsigned long currentTime = millis();

  // Update LCD
  if (empty != 0) {
    lcd.setCursor(0, 1);
    lcd.print("Emp: ");
    lcd.print(empty);
    lcd.print(" Occ: ");
    lcd.print(occupied);

    if ((digitalRead(ir_gate_in) == LOW || digitalRead(ir_gate_out) == LOW) && gatePosition == 0) {
      if (currentTime - lastGateTrigger >= debounceDelay) {
        openGate();
        lastGateTrigger = currentTime;
      }
    }
  } else {
    lcd.setCursor(0, 1);
    lcd.print("Sorry,Its full:(");
  }

  // Send data to Firebase if connected
  if (WiFi.status() == WL_CONNECTED) {
    sendParkingData(1, S1);
    sendParkingData(2, S2);
    sendParkingData(3, S3);
    sendParkingData(4, S4);
  }

  delay(100);
}

void openGate() {
  if (gatePosition == 0) {
    gate.write(90);
    gatePosition = 90;
    delay(4000);
    closeGate();
  }
}

void closeGate() {
  if (gatePosition == 90) {
    gate.write(0);
    gatePosition = 0;
  }
}

void sendParkingData(int slot, bool occupied) {
  String currentTime = getCurrentTime();
  String path = "/parking/slot" + String(slot);
  String historyPath = "/parking_history/slot" + String(slot) + "/" + String(millis());

  // Current status
  FirebaseJson json;
  json.set("occupied", occupied);
  json.set("timestamp", currentTime);

  // History entry
  FirebaseJson historyJson;
  historyJson.set("occupied", occupied);
  historyJson.set("timestamp", currentTime);

  if (Firebase.updateNode(fbData, path, json)) {
    Serial.println("Slot " + String(slot) + " data sent to Firebase");
  } else {
    Serial.println("Failed to send slot " + String(slot) + " data: " + fbData.errorReason());
  }

  if (Firebase.updateNode(fbData, historyPath, historyJson)) {
    Serial.println("History recorded for slot " + String(slot));
  } else {
    Serial.println("Failed to record history for slot " + String(slot) + ": " + fbData.errorReason());
  }
}