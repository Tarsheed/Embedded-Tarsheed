#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include "ACS712.h"
#include <SPIFFS.h>
#include <HTTPClient.h>

// بيانات WiFi
const char* ssid = "TARSHEED";
const char* password = "TARSHEED2025";

// بيانات MQTT
const char* mqtt_server = "seal-01.lmq.cloudamqp.com";
const int mqtt_port = 1883;
const char* mqtt_user = "frtmkjcl";
const char* mqtt_password = "YOUR_MQTT_PASSWORD";

WiFiClient espClient;
PubSubClient client(espClient);
HTTPClient http;

// تعريف الحساسات
#define DHT_PIN 15
#define DHT_TYPE DHT22
#define ACS_PIN 34
DHT dht(DHT_PIN, DHT_TYPE);
ACS712 acs(ACS_PIN, 5.0, 1023, 100);

// متغيرات البيانات
String userID = "";
StaticJsonDocument<2048> configData;
float lastTemperature = 0.0, lastHumidity = 0.0;
unsigned long lastEnergyReport = 0, lastReconnectAttempt = 0;
const unsigned long reportInterval = 12 * 60 * 60 * 1000;

void setup() {
    Serial.begin(115200);

    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount SPIFFS");
        return;
    }

    setup_wifi();
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);

    loadConfig();
    fetchDevicesFromBackend();

    reconnect();
}

void setup_wifi() {
    Serial.print("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected to WiFi");
}

void loadConfig() {
    File file = SPIFFS.open("/config.json", "r");
    if (!file) {
        Serial.println("Config file not found, creating new one...");
        saveConfig();
        return;
    }
    DeserializationError error = deserializeJson(configData, file);
    if (error) Serial.println("Failed to parse config file");
    file.close();
}

void saveConfig() {
    File file = SPIFFS.open("/config.json", "w");
    if (!file) {
        Serial.println("Failed to save config file");
        return;
    }
    serializeJson(configData, file);
    file.close();
}

void fetchDevicesFromBackend() {
    Serial.println("Fetching devices from backend...");
    http.begin("http://your-backend.com/devices");
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        String payload = http.getString();
        DeserializationError error = deserializeJson(configData, payload);
        if (!error) {
            saveConfig();
            Serial.println("Devices updated from backend");
        } else {
            Serial.println("Failed to parse backend response");
        }
    } else {
        Serial.println("Failed to fetch devices, HTTP Code: " + String(httpCode));
    }
    
    http.end();
}

void reconnect() {
    if (millis() - lastReconnectAttempt < 5000) return;
    lastReconnectAttempt = millis();

    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (client.connect("ESP32Client", mqtt_user, mqtt_password)) {
            Serial.println("Connected to MQTT Broker");
            client.subscribe("home/devices");
            client.publish("home/test", "ESP32 connected to CloudAMQP");
        } else {
            Serial.println("Failed, rc=" + String(client.state()));
            delay(5000);
        }
    }
}

void callback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.println("Received message: " + message);

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, message);
    if (error) {
        Serial.println("Invalid JSON format");
        return;
    }

    int pinNumber = doc["pin"];
    String command = doc["command"];

    if (command == "ON") {
        digitalWrite(pinNumber, HIGH);
        Serial.println("Turned ON device at pin: " + String(pinNumber));
    } else if (command == "OFF") {
        digitalWrite(pinNumber, LOW);
        Serial.println("Turned OFF device at pin: " + String(pinNumber));
    }
}

void readSensors() {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    if (!isnan(temperature) && !isnan(humidity)) {
        if (abs(temperature - lastTemperature) > 0.5 || abs(humidity - lastHumidity) > 2.0) {
            lastTemperature = temperature;
            lastHumidity = humidity;
            client.publish("home/temperature", String(temperature).c_str());
            client.publish("home/humidity", String(humidity).c_str());
        }
    } else {
        Serial.println("Failed to read from DHT sensor");
    }

    if (millis() - lastEnergyReport >= reportInterval) {
        lastEnergyReport = millis();
        float current = acs.mA_AC();
        client.publish("home/power", String(current).c_str());
    }
}

void loop() {
    if (!client.connected()) reconnect();
    client.loop();
    readSensors();
}
