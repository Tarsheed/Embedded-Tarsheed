#define LDR_PIN 34  

void setup() {
    Serial.begin(115200);
}

void loop() {
    int lightValue = analogRead(LDR_PIN);
    
    Serial.print("Light Level: "); Serial.println(lightValue);

    delay(1000);
}
