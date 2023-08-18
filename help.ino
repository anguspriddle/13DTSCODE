#include <Servo.h>
#include <TM1637Display.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";

Servo lockServo;
TM1637Display display(4, 5);  // CLK, DIO pins
int redLedPin = 12;
int greenLedPin = 13;

bool isLocked = false;
unsigned long unlockTime = 0;

AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);

  // Initialize components and peripherals
  lockServo.attach(2); // Change to your servo pin
  pinMode(redLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);

  // Set up WiFi connection
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Set up web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/set_timer", HTTP_POST, handleSetTimer);
  server.begin();

  // Initialize display
  display.setBrightness(0x0f);  // Set brightness (0-15)
  display.clear();
  display.showNumberDec(0);     // Display initial value
}

void loop() {
  server.handleClient();

  if (isLocked && millis() >= unlockTime) {
    lockServo.write(0); // Lock position
    digitalWrite(redLedPin, LOW);
    digitalWrite(greenLedPin, HIGH);
    isLocked = false;
    display.showNumberDec(0); // Clear display
  } else if (isLocked) {
    unsigned long remainingTime = (unlockTime - millis()) / 1000;
    display.showNumberDecEx(remainingTime, 0b01000000, true);
  }
}

void unlockSafe(unsigned long duration) {
  lockServo.write(90); // Unlock position
  digitalWrite(redLedPin, HIGH);
  digitalWrite(greenLedPin, LOW);
  isLocked = true;
  unlockTime = millis() + duration * 1000;
}

void handleRoot(AsyncWebServerRequest *request) {
  String html = "<html><body>";
  html += "<h1>Electronic Safe Control</h1>";
  html += "<form method='post' action='/set_timer'>";
  html += "Set Timer (seconds): <input type='number' name='timer' min='1'><br>";
  html += "<input type='submit' value='Set Timer'>";
  html += "</form>";
  html += "</body></html>";

  request->send(200, "text/html", html);
}

void handleSetTimer(AsyncWebServerRequest *request) {
  if (request->hasParam("timer")) {
    int timer = request->getParam("timer")->value().toInt();
    unlockSafe(timer);
  }
  request->send(303); // Redirect back to root
}
