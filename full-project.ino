#include "DHT.h"
#include <WiFi.h>
#include <AsyncMqttClient.h>
#include <esp_sleep.h>

#define WIFI_SSID "REPLACE_WITH_YOUR_SSID"
#define WIFI_PASSWORD "REPLACE_WITH_YOUR_PASSWORD"
#define MQTT_HOST IPAddress(192, 168, 1, 255)
#define MQTT_PORT 1883

// Device ID
#define DHT_SENSOR_ID "DHT22_01"
#define ACS712_SENSOR_ID "ACS712_01"
#define PIR_SENSOR_ID "PIR_01"
#define AC_DEVICE_ID "AC_01"
#define LIGHT_DEVICE_ID "LIGHT_01"

#define DHTPIN 4  
#define DHTTYPE DHT22  
#define ACS712_PIN 12   
#define PIR_PIN 7       
#define AC_PIN  8       
#define LIGHT_PIN 9     
#define NO_MOTION_TIMEOUT 600000 

DHT dht(DHTPIN, DHTTYPE);
AsyncMqttClient mqttClient;
unsigned long previousMillis = 0;
const long interval = 12 * 60 * 60 * 1000;
bool motionDetected = false;
unsigned long lastMotionTime = 0;
float VOLTAGE = 220.0;

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void checkMotion() {
  if (digitalRead(PIR_PIN) == HIGH) {
    motionDetected = true;
    lastMotionTime = millis();
    digitalWrite(AC_PIN, HIGH);
    digitalWrite(LIGHT_PIN, HIGH);
    mqttClient.publish("esp32/ac_status", 1, true, AC_DEVICE_ID ":1");
    mqttClient.publish("esp32/light_status", 1, true, LIGHT_DEVICE_ID ":1");
  } else if (millis() - lastMotionTime > NO_MOTION_TIMEOUT) {
    motionDetected = false;
    digitalWrite(AC_PIN, LOW);
    digitalWrite(LIGHT_PIN, LOW);
    mqttClient.publish("esp32/ac_status", 1, true, AC_DEVICE_ID ":0");
    mqttClient.publish("esp32/light_status", 1, true, LIGHT_DEVICE_ID ":0");
  }
}

float readCurrent() {
  int sensorValue = analogRead(ACS712_PIN);
  float voltage = sensorValue * (5.0 / 4095.0);
  float current = (voltage - 2.5) / 0.185;
  return abs(current);
}

void deepSleepMode() {
  Serial.println("Entering deep sleep mode...");
  esp_sleep_enable_timer_wakeup(6 * 60 * 60 * 1000000);
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(PIR_PIN, INPUT);
  pinMode(AC_PIN, OUTPUT);
  pinMode(LIGHT_PIN, OUTPUT);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  connectToWifi();
  connectToMqtt();
}

void loop() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;

  if (timeinfo.tm_hour >= 0 && timeinfo.tm_hour < 6) {
    deepSleepMode();
  }

  checkMotion();

  if (millis() - previousMillis >= interval) {
    previousMillis = millis();
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    float current = readCurrent();
    float power = current * VOLTAGE;
    
    mqttClient.publish("esp32/dht/temperature", 1, true, DHT_SENSOR_ID ":" + String(temp).c_str());
    mqttClient.publish("esp32/dht/humidity", 1, true, DHT_SENSOR_ID ":" + String(hum).c_str());
    mqttClient.publish("esp32/acs712/current", 1, true, ACS712_SENSOR_ID ":" + String(current).c_str());
    mqttClient.publish("esp32/acs712/power", 1, true, ACS712_SENSOR_ID ":" + String(power).c_str());
  }
}
