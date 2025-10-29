#include "TemperatureSensor.h"

// Le constructeur initialise l'objet DHT avec le bon pin et le bon type
TemperatureSensor::TemperatureSensor(uint8_t pin)
    : sensorPin(pin), dht(pin, DHT11) {}

void TemperatureSensor::begin() {
    dht.begin();
}

float TemperatureSensor::read() {
    // Lit la température (en Celsius par défaut)
    float t = dht.readTemperature();
    
    // Vérifie si la lecture a échoué (la bibliothèque renvoie NAN)
    if (isnan(t)) {
        Serial.println("Erreur de lecture du capteur DHT22 !");
        return NAN; // Renvoie NAN (Not-a-Number)
    }
    
    return t;
}