#pragma once
#include <ArduinoJson.h>
#include "actuators/Actuator.h"
#include "sensors/SoilHumiditySensor.h"
#include "sensors/TemperatureSensor.h"
#include "comms/CommManager.h"
#include "ConfigLoader.h" // Contient MAX_SOIL_SENSORS

class Follower {
public:
  
    Follower(const Config& config);
    void begin();
    void update();
    CommManager* getCommManager();
    



private:
    const Config& config; 
    
    Actuator actuator;
    SoilHumiditySensor* soilSensors[MAX_SOIL_SENSORS];
    uint8_t numSoilSensors; 
    
    TemperatureSensor* tempSensor; 
    
    CommManager comms;
    unsigned long lastTimeCheck;

    // MODIFIÉ: Renommage pour gérer les minutes
    bool alreadySentThisMinute; 
    bool timeIsSynced;
    int lastCheckedMinute; 
    uint8_t syncFails;
    unsigned long lastSyncAttemptTime;
    bool isSending; 
    uint8_t sendRetryCount; 
    const uint8_t MAX_SEND_RETRIES = 5; 
    String lastJsonPayload; 
    uint16_t txSeq = 1;
    unsigned long ackDeadlineMs = 0;
    uint16_t pendingAckSeq = 0;
    uint16_t baseRetryDelayMs = 120; // ESP-NOW

    void onDataSent(bool success);
    void sendSensorData();
    void onDataReceived(const SenderInfo& sender, const uint8_t* data, int len); 
};