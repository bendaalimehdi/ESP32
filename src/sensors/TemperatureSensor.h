#pragma once
#include <Arduino.h>
#include <DHT.h>

class TemperatureSensor {
public:
    /**
     * @brief 
     * @param pin 
     */
    TemperatureSensor(uint8_t pin);
    void begin();
    
    /**
     * @brief 
     * @return 
     */
    float read(); 

private:
    uint8_t sensorPin;
    DHT dht; 
};