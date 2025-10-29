#pragma once
#include <ArduinoJson.h> 
#include "actuators/Actuator.h"
#include "Config.h"
#include "comms/CommManager.h"

class Master {
public:
    Master();
    void begin();
    void update();

private:
    Actuator actuator;
    CommManager comms;
    float lastReceivedHumidity;

    // Document JSON pour le parsing (réutilisé pour économiser la mémoire)
    StaticJsonDocument<MAX_PAYLOAD_SIZE> jsonDoc;
    
    // MODIFIÉ: Accepte un buffer (data) et sa longueur (len)
    void onDataReceived(const SenderInfo& sender, const uint8_t* data, int len);
};