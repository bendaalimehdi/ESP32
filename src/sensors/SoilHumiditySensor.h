#pragma once
#include <Arduino.h>

class SoilHumiditySensor {
public:

    SoilHumiditySensor(uint8_t sensorPin, uint8_t powerPin, uint16_t dry, uint16_t wet);
    
    void begin();
    float read();

private:
    uint8_t _sensorPin;
    uint8_t _powerPin; 
    uint16_t _dryValue;
    uint16_t _wetValue;
};