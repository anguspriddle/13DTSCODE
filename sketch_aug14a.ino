#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <TM1637Display.h>

// Pin definitions
const int servoPin = 13;  // Servo control pin
const int greenLedPin = 12;  // Green LED pin
const int redLedPin = 13;    // Red LED pin
const int clkPin = 32;  // CLK pin for TM1637
const int dataPin = 33; // DIO pin for TM1637

// Create objects
WebServer server(80);
Servo lockServo;
TM1637Display display(clkPin, dataPin);

// Variables
const int defaultPasscode = 1234;
int passcode = defaultPasscode;
bool isLocked = true;
bool timedLockActive = false;
unsigned long unlockTime = 0;
unsigned long unlockDuration;

void setup() {
  // Initialize hardware
  Serial.begin(115200);
  lockServo.attach(servoPin);
  pinMode(greenLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);

  // Connect to Wi-Fi
  WiFi.begin("Hot", "password");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println("IP address: " + WiFi.localIP().toString());

  // Set up web server routes
  server.on("/", handleRoot);
  server.on("/unlock", handleUnlock);

  // Start the server
  server.begin();

  // Initialize display
  display.setBrightness(0x0f);  // Adjust brightness
  display.showNumberDec(0, false); // Display 0 initially
}

void loop() {
  server.handleClient();

  // Handle timed lock countdown
  if (timedLockActive && isLocked) {
    unsigned long currentTime = millis();
    if (currentTime - unlockTime >= unlockDuration) {
      lockServo.write(0); // Lock the servo
      isLocked = true;
      timedLockActive = false;
    } else {
      unsigned long remainingTime = (unlockDuration - (currentTime - unlockTime)) / 1000;
      display.showNumberDecEx(remainingTime, 0b01000000, true);
    }
  }
}

void handleRoot() {
  String html = "<html><body>";
  html += "<h1>Electronic Timed Lock</h1>";
  html += "<form action='/unlock' method='POST'>";
  html += "Time: <input type='text' name='inputtime'><br>";
  html += "Password: <input type='password' name='passcode'><br>";
  String lockButtonText = isLocked ? "Unlock" : "Lock";
  html += "<input type='submit' value='" + lockButtonText + "'>";

  html += "</form>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleUnlock() {
  if (server.method() == HTTP_POST) {
    String submittedPasscode = server.arg("passcode");
    String submittedTime = server.arg("inputtime");
    unsigned long submittedTimeInt = submittedTime.toInt();
    unsigned long currentTime = millis(); // Declare and initialize currentTime
    unsigned long timerEndTime = unlockTime + unlockDuration; // Calculate timerEndTime

    if (submittedPasscode.toInt() == passcode) {
      if (!isLocked) {
        lockServo.write(90); // Unlock the servo
        unlockDuration = submittedTimeInt * 1000;
        digitalWrite(greenLedPin, HIGH); // Turn on green LED
        digitalWrite(redLedPin, LOW); // Turn off red LED

        // Check if timer is running and display remaining time
        if (currentTime < timerEndTime) {
          unsigned long remainingTime = (timerEndTime - currentTime) / 1000; // Convert to seconds
          display.showNumberDecEx(remainingTime, 0b01000000, true); // Display remaining time
        }
      } else {
        unlockTime = currentTime;
        timedLockActive = true;
      }
      server.send(200, "text/plain", "Unlocked");
    } else {
      server.send(403, "text/plain", "Access Denied");
    }
  }
}
