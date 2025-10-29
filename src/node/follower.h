#pragma once
#include <ArduinoJson.h>
#include "actuators/Actuator.h"
#include "sensors/SoilHumiditySensor.h"
#include "sensors/TemperatureSensor.h"
#include "comms/CommManager.h"
#include "ConfigLoader.h" 

class Follower {
public:
    // Le constructeur prend la config en argument
    Follower(const Config& config);
    void begin();
    void update();

private:
    const Config& config; 
    
    Actuator actuator;
    SoilHumiditySensor sensor;
    TemperatureSensor tempSensor;
    CommManager comms;
    
    unsigned long lastSendTime;

    void onDataSent(bool success);
};