#pragma once
#include <ArduinoJson.h>
#include "actuators/Actuator.h"
#include "comms/CommManager.h"
#include "actuators/Electrovanne.h"
#include "comms/WifiManager.h"
#include "ConfigLoader.h"
#include "logic/IrrigationManager.h"

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
    WifiManager wifi;
    float lastReceivedHumidity;

    Electrovanne valve1;
    Electrovanne valve2;
    Electrovanne valve3;
    Electrovanne valve4;
    Electrovanne valve5;

    Electrovanne* valveArray[MAX_SOIL_SENSORS]; 
 
    IrrigationManager* irrigationManager;


    StaticJsonDocument<MAX_PAYLOAD_SIZE> jsonDoc;
    
    void onDataReceived(const SenderInfo& sender, const uint8_t* data, int len);

    

    void onMqttCommandReceived(char* topic, byte* payload, unsigned int length);
};