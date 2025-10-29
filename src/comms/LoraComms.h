#pragma once
#include <SPI.h>
#include <LoRa.h>
#include <functional>
#include "SharedStructures.h" // Pour LoraPins

class LoraComms {
public:

    using DataRecvCallback = std::function<void(const uint8_t* data, int len, uint8_t from)>;

    LoraComms(uint8_t localAddress);

    bool begin(long frequency, const LoraPins& pins, uint8_t syncWord);
    void registerRecvCallback(DataRecvCallback cb);
    
    bool sendData(uint8_t destAddress, const uint8_t* data, int len);
    void startReceiving();

private:
    static void onDataRecv_static(int packetSize);
    void handleDataRecv(int packetSize);

    static LoraComms* instance; 
    uint8_t localAddress;
    DataRecvCallback onDataReceived;
};