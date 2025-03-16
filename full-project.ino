#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include "ACS712.h"
const char* ssid = "SMART_HOME";
const char* password = "SMART_HOME2025";
const char* mqtt_server = "YOUR_MQTT_BROKER_IP";
WiFiClient espClient;
PubSubClient client(espClient);
#define LED_PIN  5      
#define AC_PIN   18      
#define FRIDGE_PIN 19    
#define TV_PIN  21      
#define PIR_PIN  23      
#define DHT_PIN  15      
#define DHT_TYPE DHT22   
DHT dht(DHT_PIN, DHT_TYPE);
#define ACS_PIN  34  
ACS712 acs(ACS_PIN, 5.0, 1023, 100);
const char* LED_ID = "device_001";
const char* AC_ID = "device_002";
const char* FRIDGE_ID = "device_003";
const char* TV_ID = "device_004";

float lastTemperature = 0.0;
float lastHumidity = 0.0;
bool lastMotionState = false;
unsigned long lastPowerReport = 0;
const unsigned long powerReportInterval = 12 * 60 * 60 * 1000; // 12 ساعة بالمللي ثانية

void setup() {
    Serial.begin(115200);
    
    pinMode(LED_PIN, OUTPUT);
    pinMode(AC_PIN, OUTPUT);
    pinMode(FRIDGE_PIN, OUTPUT);
    pinMode(TV_PIN, OUTPUT);
    pinMode(PIR_PIN, INPUT);
    
    dht.begin();
    
    setup_wifi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
}

void setup_wifi() {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    Serial.println("WiFi connected!");
}

void callback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    if (String(topic) == "home/led") {
        controlDevice(LED_PIN, message.toInt(), "LED", LED_ID);
    }
    else if (String(topic) == "home/ac") {
        controlDevice(AC_PIN, message.toInt(), "AC", AC_ID);
    }
    else if (String(topic) == "home/fridge") {
        controlDevice(FRIDGE_PIN, message.toInt(), "Fridge", FRIDGE_ID);
    }
    else if (String(topic) == "home/tv") {
        controlDevice(TV_PIN, message.toInt(), "TV", TV_ID);
    }
}

void controlDevice(int pin, int state, String deviceName, const char* deviceId) {
    digitalWrite(pin, state);
    String statusMsg = String("{") + "\"id\":\"" + deviceId + "\",\"state\":\"" + (state ? "ON" : "OFF") + "\"}";
    client.publish("home/status", statusMsg.c_str());
}

void reconnect() {
    while (!client.connected()) {
        if (client.connect("ESP32Client")) {
            client.subscribe("home/led");
            client.subscribe("home/ac");
            client.subscribe("home/fridge");
            client.subscribe("home/tv");
        } else {
            delay(5000);
        }
    }
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    readSensors();
}

void readSensors() {
    bool motionState = digitalRead(PIR_PIN);
    if (motionState != lastMotionState) {
        lastMotionState = motionState;
        String motionMsg = motionState ? "{\"motion\":\"detected\"}" : "{\"motion\":\"none\"}";
        client.publish("home/motion", motionMsg.c_str());
    }

    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    if (abs(temperature - lastTemperature) > 0.5 || abs(humidity - lastHumidity) > 2.0) {
        lastTemperature = temperature;
        lastHumidity = humidity;
        String tempMsg = "{\"temperature\":\"" + String(temperature) + "\"}";
        String humMsg = "{\"humidity\":\"" + String(humidity) + "\"}";
        client.publish("home/temperature", tempMsg.c_str());
        client.publish("home/humidity", humMsg.c_str());
    }

    if (millis() - lastPowerReport >= powerReportInterval) {
        lastPowerReport = millis();
        float current = acs.mA_AC();  
        String energyMsg = "{\"power_consumption\":\"" + String(current) + " mA\"}";
        client.publish("home/power", energyMsg.c_str());
    }
}
