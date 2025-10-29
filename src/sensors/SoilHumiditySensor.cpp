#include "SoilHumiditySensor.h"



SoilHumiditySensor::SoilHumiditySensor(uint8_t sensorPin, uint8_t powerPin, uint16_t dry, uint16_t wet)
    : _sensorPin(sensorPin), _powerPin(powerPin), _dryValue(dry), _wetValue(wet) {}

void SoilHumiditySensor::begin() {
    pinMode(_sensorPin, INPUT);
    pinMode(_powerPin, OUTPUT);
    digitalWrite(_powerPin, LOW);
}

float SoilHumiditySensor::read() {

    digitalWrite(_powerPin, HIGH);
    delay(50); 
    int rawValue = analogRead(_sensorPin);
    digitalWrite(_powerPin, LOW);
    int humidity = map(rawValue, _dryValue, _wetValue, 0, 100);
    humidity = constrain(humidity, 0, 100);
    return (float)humidity;
}