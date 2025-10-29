#pragma once
#include <ArduinoJson.h> // Ajouté
#include "actuators/Actuator.h"
#include "sensors/SoilHumiditySensor.h"
#include "sensors/TemperatureSensor.h"
// #include "SharedStructures.h" // N'est plus nécessaire
#include "Config.h"
#include "comms/CommManager.h"

class Follower {
public:
    Follower();
    void begin();
    void update();

private:
    Actuator actuator;
    SoilHumiditySensor sensor;
    TemperatureSensor tempSensor;
    CommManager comms;
    
   
    unsigned long lastSendTime;

    void onDataSent(bool success);
};