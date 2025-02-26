#include <Arduino.h>

#define ACS712_PIN 34   
#define VREF 3.3        
#define ADC_RES 4095.0  
#define SENSITIVITY 0.185  

void setup() {
    Serial.begin(115200);
}

void loop() {
    int rawValue = analogRead(ACS712_PIN); 
    float voltage = (rawValue / ADC_RES) * VREF;  
    float current = (voltage - (VREF / 2)) / SENSITIVITY; 

    float power = current * 220.0; 
    float energy = power / 1000.0; 

    Serial.print("Current: "); Serial.print(current); Serial.println(" A");
    Serial.print("Power: "); Serial.print(power); Serial.println(" W");
    Serial.print("Energy: "); Serial.print(energy); Serial.println(" kWh");
    
    delay(2000);
}
