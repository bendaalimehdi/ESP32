#pragma once
#include <ArduinoJson.h>
#include "actuators/Actuator.h"
#include "comms/CommManager.h"
#include "actuators/Electrovanne.h"
#include "comms/WifiManager.h"
#include "ConfigLoader.h"
#include "logic/IrrigationManager.h"
#include <vector>
#include <string>
#define MAX_VALVES 20

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
    Electrovanne* valveArray[MAX_VALVES] = {nullptr};
    size_t numValves = 0;
    




 
    IrrigationManager* irrigationManager;


    StaticJsonDocument<MAX_PAYLOAD_SIZE> jsonDoc;
    std::vector<std::string> telemetryQueue;
    const size_t MAX_QUEUE_SIZE = 20; 

    unsigned long lastWifiAttempt = 0;
    static constexpr unsigned long WIFI_RETRY_INTERVAL_MS = 10UL * 60UL * 1000UL; // 10 minutes
    bool wifiReady = false;

    
    void onDataReceived(const SenderInfo& sender, const uint8_t* data, int len);

    

    void onMqttCommandReceived(char* topic, byte* payload, unsigned int length);
};