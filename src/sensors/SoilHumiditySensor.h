#pragma once
#include <Arduino.h>

class SoilHumiditySensor {
public:
    // Constructeur mis à jour
    SoilHumiditySensor(uint8_t sensorPin, uint8_t powerPin, uint16_t dry, uint16_t wet);
    
    void begin();
    
    // Lit le capteur et retourne un pourcentage (0.0 - 100.0)
    float read(); // Gardé en float pour être compatible avec le reste du code

private:
    uint8_t _sensorPin;
    uint8_t _powerPin; // <-- AJOUT
    uint16_t _dryValue;
    uint16_t _wetValue;
};