#define REED_PIN 14  

void setup() {
    Serial.begin(115200);
    pinMode(REED_PIN, INPUT_PULLUP);
}

void loop() {
    int windowState = digitalRead(REED_PIN);

    if (windowState == LOW) {
        Serial.println("Window is OPEN!");  
    } else {
        Serial.println("Window is CLOSED.");
    }

    delay(1000);
}
