#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include "ACS712.h"

// بيانات شبكة WiFi
const char* ssid = "TARSHEED";
const char* password = "TARSHEED2025";
const char* mqtt_server = "YOUR_MQTT_BROKER_IP";

WiFiClient espClient;
PubSubClient client(espClient);

// تعريف البنات للأجهزة المختلفة
#define LED_PIN 5
#define AC_PIN 18
#define FRIDGE_PIN 19
#define TV_PIN 21
#define PIR_PIN 23
#define DHT_PIN 15
#define DHT_TYPE DHT22
#define ACS_PIN 34

DHT dht(DHT_PIN, DHT_TYPE);
ACS712 acs(ACS_PIN, 5.0, 1023, 100);

// معرفات الأجهزة
const char* LED_ID = "device_led";
const char* AC_ID = "device_ac";
const char* FRIDGE_ID = "device_fridge";
const char* TV_ID = "device_tv";

float lastTemperature = 0.0;
float lastHumidity = 0.0;
bool lastMotionState = false;
unsigned long lastEnergyReport = 0;
const unsigned long reportInterval = 12 * 60 * 60 * 1000;

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    pinMode(AC_PIN, OUTPUT);
    pinMode(FRIDGE_PIN, OUTPUT);
    pinMode(TV_PIN, OUTPUT);
    pinMode(PIR_PIN, INPUT);
    digitalWrite(LED_PIN, LOW);
    digitalWrite(AC_PIN, LOW);
    digitalWrite(FRIDGE_PIN, LOW);
    digitalWrite(TV_PIN, LOW);
    dht.begin();
    setup_wifi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
}

void setup_wifi() {
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected!");
}

void callback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.print("Received message: ");
    Serial.println(message);

    if (String(topic) == "home/led") {
        controlDevice(LED_PIN, message.toInt(), LED_ID);
    } else if (String(topic) == "home/ac") {
        controlDevice(AC_PIN, message.toInt(), AC_ID);
    } else if (String(topic) == "home/fridge") {
        controlDevice(FRIDGE_PIN, message.toInt(), FRIDGE_ID);
    } else if (String(topic) == "home/tv") {
        controlDevice(TV_PIN, message.toInt(), TV_ID);
    }
}

void controlDevice(int pin, int state, const char* deviceId) {
    digitalWrite(pin, state);
    Serial.printf("%s state changed to: %s\n", deviceId, state ? "ON" : "OFF");
}

void reconnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (client.connect("ESP32Client")) {
            Serial.println("connected");
            client.subscribe("home/led");
            client.subscribe("home/ac");
            client.subscribe("home/fridge");
            client.subscribe("home/tv");
        } else {
            Serial.print("failed, rc=");
            Serial.println(client.state());
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
        String motionMsg = motionState ? "تم اكتشاف حركة!" : "لا يوجد حركة.";
        client.publish("home/motion", motionMsg.c_str());
        Serial.println(motionMsg);
    }

    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    if (abs(temperature - lastTemperature) > 0.5 || abs(humidity - lastHumidity) > 2.0) {
        lastTemperature = temperature;
        lastHumidity = humidity;
        String tempMsg = "درجة الحرارة: " + String(temperature) + "°C";
        String humMsg = "الرطوبة: " + String(humidity) + "%";
        client.publish("home/temperature", tempMsg.c_str());
        client.publish("home/humidity", humMsg.c_str());
        Serial.println(tempMsg);
        Serial.println(humMsg);
    }

    if (millis() - lastEnergyReport >= reportInterval) {
        lastEnergyReport = millis();
        float current = acs.mA_AC();  
        String energyMsg = "استهلاك الكهرباء الحالي: " + String(current) + " mA";
        client.publish("home/power", energyMsg.c_str());
        Serial.println(energyMsg);
    }
}
