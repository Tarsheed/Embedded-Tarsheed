      #include <WiFi.h>
      #include <WiFiClientSecure.h>
      #include <PubSubClient.h>
      #include <DHT.h>
      #include <ArduinoJson.h>
      #include "ACS712.h"
      #include <SPIFFS.h>

      const char* ssid = "Anas";
      const char* password = "ANAS2525++++++";

      const char* mqtt_server = "16e4e8df7324425ca5345bf26bd5aeec.s1.eu.hivemq.cloud";
      const int mqtt_port = 8883;
      const char* mqtt_user = "ahmed_1";
      const char* mqtt_pass = "Arnousa712@2002#";

      #define DHT_PIN 17
      #define DHT_TYPE DHT22
      #define ACS_PIN 34
      #define RELAY_PIN 26
      // #define LED_PIN 27
      #define PIR_PIN 4
      #define VIBRATION_PIN 21


      const char* email = "mohamedoessam32@gmail.com";

      WiFiClientSecure espClient;
      PubSubClient client(espClient);
      DHT dht(DHT_PIN, DHT_TYPE);
      ACS712 acs(ACS_PIN, 5.0, 1023, 100);
      StaticJsonDocument<2048> configData;

      float lastTemperature = 0.0, lastHumidity = 0.0;
      unsigned long lastEnergyReport = 0;
      const unsigned long reportInterval = 30 * 1000;

      void setup() {
        Serial.begin(115200);
        pinMode(RELAY_PIN, OUTPUT);
        // pinMode(LED_PIN, OUTPUT);
        pinMode(PIR_PIN, INPUT);
        pinMode(VIBRATION_PIN, INPUT);
        SPIFFS.begin(true);
        setup_wifi();
        espClient.setInsecure();
        client.setServer(mqtt_server, mqtt_port);
        client.setCallback(callback);
        loadConfig();
        reconnect();
      }

      void setup_wifi() {
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) {
          delay(500);
          Serial.print(".");
        }
        Serial.println("\nWiFi connected");
      }

      void loadConfig() {
        File file = SPIFFS.open("/config.json", "r");
        if (file) {
          deserializeJson(configData, file);
          file.close();
        }
      }

      void reconnect() {
        while (!client.connected()) {
          if (client.connect("ESP32Client", mqtt_user, mqtt_pass)) {
            client.subscribe("device/airConditioner");
            client.subscribe("device/led");
            client.publish("home/status", "ESP32 Online");
          } else {
            delay(3000);
          }
        }
      }

      void callback(char* topic, byte* payload, unsigned int length) {
        String message;
        for (unsigned int i = 0; i < length; i++) message += (char)payload[i];
        StaticJsonDocument<256> doc;
        if (deserializeJson(doc, message)) return;
        String topicStr = String(topic);

        if (topicStr == "device/airConditioner") {
          int pin = doc["pin"];
          String status = doc["status"];
          if (status == "ON") {
            digitalWrite(pin, HIGH);
            // digitalWrite(LED_PIN, HIGH);
          } else {
            digitalWrite(pin, LOW);
            // digitalWrite(LED_PIN, LOW);
          }
        } else if (topicStr == "device/led") {
          JsonArray pins = doc["pins"];
          String status = doc["status"];
          for (int i = 0; i < pins.size(); i++) {
            int pin = pins[i];
            pinMode(pin, OUTPUT);
            digitalWrite(pin, status == "ON" ? HIGH : LOW);
          }
        }

        client.publish("log/callback", message.c_str());
      }

      void readSensors() {
        float temp = dht.readTemperature();
        float hum = dht.readHumidity();
        StaticJsonDocument<128> json;

        if (abs(temp - lastTemperature) > 0.5 || abs(hum - lastHumidity) > 2.0) {
          lastTemperature = temp;
          lastHumidity = hum;
          json.clear();
          json["pin"] = DHT_PIN;
          json["email"] = email;
          json["temp"] = temp;
          json["hum"] = hum;
          String payload;
          serializeJson(json, payload);
          client.publish("sensor/temp", payload.c_str());
        }

        if (millis() - lastEnergyReport > reportInterval) {
          lastEnergyReport = millis();
          float current = acs.mA_AC();
          json.clear();
          json["pin"] = ACS_PIN;
          json["email"] = email;
          json["usage"] = current;
          String payload;
          serializeJson(json, payload);
          client.publish("sensor/reading", payload.c_str());
        }

      bool motionDetected = digitalRead(PIR_PIN);
        json.clear();
        json["pin"] = PIR_PIN;
        json["email"] = email;

        if (motionDetected) {
          json["event"] = "Motion detected";
          String motionPayload;
          serializeJson(json, motionPayload);
          client.publish("triger/movement", motionPayload.c_str());
        } else {
          json["event"] = "No motion";
          String motionPayload;
          serializeJson(json, motionPayload);
          client.publish("triger/noMovement", motionPayload.c_str());
        }

      // الاهتزاز - باستخدام if else مع event و topic مختلف
      bool vibrationDetected = digitalRead(VIBRATION_PIN);
      json.clear();
      json["pin"] = VIBRATION_PIN;
      json["email"] = email;

      if (vibrationDetected) {
        json["event"] = "Vibration detected";
        String vibrationPayload;
        serializeJson(json, vibrationPayload);
        client.publish("sensor/vibration", vibrationPayload.c_str());
      } else {
        json["event"] = "No vibration";
        String vibrationPayload;
        serializeJson(json, vibrationPayload);
        client.publish("sensor/noVibration", vibrationPayload.c_str());
      }

      }

      void loop() {
        if (!client.connected()) reconnect();
        client.loop();
        readSensors();
      }
