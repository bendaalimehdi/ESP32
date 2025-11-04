#pragma once
#include <ArduinoJson.h>
#include "actuators/Actuator.h"
#include "comms/CommManager.h"
#include "ConfigLoader.h"

class Master {
public:
    Master(const Config& config);
    void begin();
    void update();

    CommManager* getCommManager();

private:
    const Config& config; 

    Actuator actuator;
    CommManager comms;
    float lastReceivedHumidity;


    StaticJsonDocument<MAX_PAYLOAD_SIZE> jsonDoc;
    
    void onDataReceived(const SenderInfo& sender, const uint8_t* data, int len);
};