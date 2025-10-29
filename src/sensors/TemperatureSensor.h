#pragma once
#include <Arduino.h>
#include <DHT.h>

class TemperatureSensor {
public:
    /**
     * @brief Constructeur.
     * @param pin Le pin de données (DATA) du DHT22.
     */
    TemperatureSensor(uint8_t pin);
    void begin();
    
    /**
     * @brief Lit la température depuis le capteur.
     * @return Température en Celsius, ou NAN en cas d'échec.
     */
    float read(); 

private:
    uint8_t sensorPin;
    DHT dht; // L'objet de la bibliothèque DHT
};