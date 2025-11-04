#pragma once
#include "comms/ESPNowComms.h"
#include "comms/LoraComms.h"
#include "ConfigLoader.h"

enum class CommMode {
    NONE,
    ESP_NOW,
    LORA
};

struct SenderInfo {
    CommMode mode;
    const uint8_t* macAddress;
    uint8_t loraAddress;
};


class CommManager {
public:
    using DataRecvCallback = std::function<void(const SenderInfo& sender, const uint8_t* data, int len)>;
    using SendStatusCallback = std::function<void(bool success)>;

    CommManager(); // Constructeur mis à jour

    bool begin(const ConfigNetwork& netConfig, const ConfigPins& pinConfig, bool isMaster);

    void registerRecvCallback(DataRecvCallback cb);
    void registerSendCallback(SendStatusCallback cb);
    bool sendData(const char* jsonData);
    void update();
    
    CommMode getActiveMode() const { return activeMode; }

private:
    ESPNowComms espNow;
    LoraComms lora;
    CommMode activeMode;

    const uint8_t* espnowPeerMac;
    uint16_t loraPeerAddress; 

    DataRecvCallback userRecvCallback;
    SendStatusCallback userSendCallback;

    void onEspNowDataRecv(const uint8_t* mac, const uint8_t* data, int len);
    void onEspNowSendStatus(bool success);
    void onLoraDataRecv(const uint8_t* data, int len, uint16_t from); 
};