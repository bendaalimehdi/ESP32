#pragma once
#include <HardwareSerial.h>
#include <functional>
#include "ConfigLoader.h"
#include "actuators/Actuator.h"

class LoraComms {
public:

    using DataRecvCallback = std::function<void(const uint8_t* data, int len, uint16_t from)>;

    LoraComms(HardwareSerial& serial, Actuator* actuator);
    bool begin(const ConfigPins& pinConfig, const ConfigNetwork& netConfig);
    void registerRecvCallback(DataRecvCallback cb);
    

    bool sendData(uint16_t destAddress, const uint8_t* data, int len);
    

    void update();

private:
    HardwareSerial& loraSerial; 
    ConfigPins pins;
    uint8_t channel; // Stocke le canal de communication
    
    DataRecvCallback onDataReceived;
    Actuator* actuator;
    
    void setMode(uint8_t mode);
    bool waitForAux(unsigned long timeout = 1000);
    bool configureModule(const ConfigNetwork& netConfig);
    

    
    uint8_t rxBuffer[MAX_PAYLOAD_SIZE];
};