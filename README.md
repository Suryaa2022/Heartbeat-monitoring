# Heartbeat-monitoring

// Include the necessary libraries
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <PulseSensorPlayground.h>

// Initialize the OLED display
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

// Initialize the pulse sensor
const int pulsePin = A0;
const int blinkPin = 13;
PulseSensorPlayground pulseSensor;

void setup() {
  // Initialize the serial monitor
  Serial.begin(9600);
  
  // Initialize the OLED display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Heartbeat Monitor");
  display.display();
  
  // Initialize the pulse sensor
  pulseSensor.analogInput(pulsePin);
  pulseSensor.blinkOnPulse(blinkPin);
  pulseSensor.setThreshold(550);
  pulseSensor.begin();
}

void loop() {
  // Update the pulse sensor
  pulseSensor.update();
  
  // Retrieve the heart rate
  int heartRate = pulseSensor.getBeatsPerMinute();
  
  // Print the heart rate to the serial monitor
  Serial.print("Heart Rate: ");
  Serial.println(heartRate);
  
  // Display the heart rate on the OLED display
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Heart Rate: ");
  display.println(heartRate);
  display.display();
  
  // Wait for a moment before the next reading
  delay(10);
}
