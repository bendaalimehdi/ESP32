#include "TemperatureSensor.h"


TemperatureSensor::TemperatureSensor(uint8_t pin)
    : sensorPin(pin), dht(pin, DHT11) {}

void TemperatureSensor::begin() {
    dht.begin();
}

float TemperatureSensor::read() {

    float t = dht.readTemperature();
    
    if (isnan(t)) {
        Serial.println("Erreur de lecture du capteur DHT11 !");
        return NAN;
    }
    
    return t;
}