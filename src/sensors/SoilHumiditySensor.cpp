#include "SoilHumiditySensor.h"
#include "Config.h" // Pour les valeurs de calibration

// Constructeur mis à jour
SoilHumiditySensor::SoilHumiditySensor(uint8_t sensorPin, uint8_t powerPin, uint16_t dry, uint16_t wet)
    : _sensorPin(sensorPin), _powerPin(powerPin), _dryValue(dry), _wetValue(wet) {}

void SoilHumiditySensor::begin() {
    pinMode(_sensorPin, INPUT);
    // Configurer le pin d'alimentation en sortie et l'éteindre par défaut
    pinMode(_powerPin, OUTPUT);
    digitalWrite(_powerPin, LOW);
}

float SoilHumiditySensor::read() {
    // 1. Allumer le capteur
    digitalWrite(_powerPin, HIGH);
    delay(50); // Attendre que le capteur se stabilise

    // 2. Lire la valeur brute
    int rawValue = analogRead(_sensorPin);

    // 3. Éteindre le capteur (très important)
    digitalWrite(_powerPin, LOW);

    // 4. Mapper et contraindre la valeur (votre logique)
    // J'utilise _dryValue et _wetValue pour garder la calibration de Config.h
    int humidity = map(rawValue, _dryValue, _wetValue, 0, 100);
    humidity = constrain(humidity, 0, 100);

    return (float)humidity; // Convertir en float pour le reste du système
}