#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

Servo gate;
const int servoPin = 2;
const int ir_gate_in = 15;
const int ir_gate_out = 4;

// IR sensors for the 4 parking slots
const int ir_car1 = 12;  // Slot 1
const int ir_car2 = 14;  // Slot 2
//const int ir_car3 = 27;  // Slot 3
//const int ir_car4 = 26;  // Slot 4

// Variables for parking slots (1 = occupied, 0 = empty)
int S1 = 0, S2 = 0;//, S3 = 0, S4 = 0;
int gatePosition = 0;  // 0 = closed, 90 = open

// Initialize with I2C address (try 0x27 or 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Wire.begin(21, 22);  // SDA = 21, SCL = 22
  lcd.init();          // Initialize the LCD
  lcd.backlight();     // Turn on backlight
  lcd.setCursor(0, 0);
  lcd.print("Smart Parking");

  pinMode(ir_gate_in, INPUT);
  pinMode(ir_gate_out, INPUT);
  
  pinMode(ir_car1, INPUT);
  pinMode(ir_car2, INPUT);
  //pinMode(ir_car3, INPUT);
  //pinMode(ir_car4, INPUT);

  gate.attach(servoPin);
  gate.write(0);  // Start with the gate closed

  // Show initial parking status
  lcd.setCursor(0, 1);
  lcd.print("Emp: 2 Occ: 0 ");
}

void loop() {
  // Read parking sensor states
  S1 = digitalRead(ir_car1);  // Check Slot 1
  S2 = digitalRead(ir_car2);  // Check Slot 2
  //S3 = digitalRead(ir_car3);  // Check Slot 3
  //S4 = digitalRead(ir_car4);  // Check Slot 4

  // Check for car entering
  if (digitalRead(ir_gate_in) == LOW && gatePosition == 0) {  // Car entering and gate is closed
    openGate();
  }

  // Check for car exiting
  if (digitalRead(ir_gate_out) == LOW && gatePosition == 0) {  // Car exiting and gate is closed
    openGate();
  }

  // Calculate the total number of occupied slots
  int empty = S1 + S2;// + S3 + S4;  // Sum of all occupied slots (1 for occupied, 0 for empty)
  int occupied = 2 - empty;  // Total empty slots

  // Display the total number of occupied and empty slots on the LCD
  lcd.setCursor(0, 1);
  lcd.print("Emp: ");
  lcd.print(empty);     // Display the total number of empty slots
  lcd.print(" Occ: ");
  lcd.print(occupied);  // Display the total number of occupied slots
 

  delay(100);  // Short delay to reduce the loop frequency
}

void openGate() {
  gate.write(90);  // Open the gate
  gatePosition = 90;
  delay(3000);     // Keep it open for 3 seconds
  closeGate();
}

void closeGate() {
  if (gatePosition == 90) {  // Close only if it was open
    gate.write(0);  // Close the gate
    gatePosition = 0;
  }
}
