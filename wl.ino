#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "Water_Level_System";
const char* password = "password123";

WebServer server(80);

const int sensorPin = 34; 
const int motorPin = 5;   
const int buzzerPin = 32; 

const int ledPins[] = {13, 12, 14, 27, 26, 25, 33}; 
const int numLeds = 7;

bool motorState = false;
int waterPercentage = 0;

void setup() {
  Serial.begin(115200);

  pinMode(motorPin, OUTPUT);
  digitalWrite(motorPin, LOW);
  
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  for (int i = 0; i < numLeds; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }

  Serial.println("Starting Access Point...");
  WiFi.softAP(ssid, password);
  
  server.on("/status", HTTP_GET, []() {
    String json = "{";
    json += "\"waterLevel\":" + String(waterPercentage) + ",";
    json += "\"motor\":" + String(motorState ? "true" : "false");
    json += "}";
    server.send(200, "application/json", json);
  });

  server.on("/motor/on", HTTP_GET, []() {
    motorState = true;
    digitalWrite(motorPin, HIGH); 
    server.send(200, "text/plain", "Motor ON");
  });

  server.on("/motor/off", HTTP_GET, []() {
    motorState = false;
    digitalWrite(motorPin, LOW); 
    server.send(200, "text/plain", "Motor OFF");
  });

  server.begin();
}

void loop() {
  server.handleClient();

  // ONLY READ SENSORS IF THE MOTOR IS TURNED ON
  if (motorState == true) {
    int rawValue = analogRead(sensorPin);
    
    // Map the raw analog value to a 0-100 percentage.
    // NOTE: If your sensor never reaches 100% in the app, lower '4095' to your actual max (like 1800).
    int compressedValue = map(rawValue, 0, 4095, 0, 100);
    
    // Safety bounds
    if (compressedValue < 0) compressedValue = 0;
    if (compressedValue > 100) compressedValue = 100;
    
    // Update the global variable so the Android App can see it
    waterPercentage = compressedValue;

    // --- NON-LINEAR CUSTOM LED LOGIC ---
    int ledsToLight = 0;
    bool soundBuzzer = false;

    // Evaluates top-down, exactly matching your requested thresholds
    if (compressedValue >= 90) {
      ledsToLight = 7;
      soundBuzzer = true;
    } else if (compressedValue >= 82) {
      ledsToLight = 5;
    } else if (compressedValue >= 76) {
      ledsToLight = 4;
    } else if (compressedValue >= 70) {
      ledsToLight = 3;
    } else if (compressedValue >= 65) {
      ledsToLight = 2;
    } else if (compressedValue >= 60) {
      ledsToLight = 1;
    } else {
      // If compressedValue is less than 60
      ledsToLight = 0;
    }

    // Apply the LED states to the hardware pins
    for (int i = 0; i < numLeds; i++) {
      if (i < ledsToLight) {
        digitalWrite(ledPins[i], HIGH);
      } else {
        digitalWrite(ledPins[i], LOW);
      }
    }
    
    // Apply buzzer state
    if (soundBuzzer) {
      digitalWrite(buzzerPin, HIGH);
    } else {
      digitalWrite(buzzerPin, LOW);
    }
    
  } 
  // IF MOTOR IS OFF, STANDBY MODE
  else {
    waterPercentage = 0; 
    for (int i = 0; i < numLeds; i++) {
      digitalWrite(ledPins[i], LOW); 
    }
    digitalWrite(buzzerPin, LOW); 
  }

  delay(50);
}
