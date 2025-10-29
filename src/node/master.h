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

private:
    const Config& config; // Stocke une référence à la config

    Actuator actuator;
    CommManager comms;
    float lastReceivedHumidity;

    // Document JSON pour le parsing
    StaticJsonDocument<MAX_PAYLOAD_SIZE> jsonDoc;
    
    void onDataReceived(const SenderInfo& sender, const uint8_t* data, int len);
};