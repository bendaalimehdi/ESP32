#pragma once
#include "comms/ESPNowComms.h"
#include "comms/LoraComms.h"
#include "Config.h" 

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
    // MODIFIÉ: Le callback reçoit un buffer brut (data) et sa longueur (len)
    using DataRecvCallback = std::function<void(const SenderInfo& sender, const uint8_t* data, int len)>;
    using SendStatusCallback = std::function<void(bool success)>;

    CommManager(uint8_t loraLocalAddr);
    bool begin(const uint8_t* espnowPeerMac, uint8_t loraPeerAddr);

    void registerRecvCallback(DataRecvCallback cb);
    void registerSendCallback(SendStatusCallback cb);
    
    // MODIFIÉ: Prend une chaîne JSON en entrée
    bool sendData(const char* jsonData);
    
    CommMode getActiveMode() const { return activeMode; }

private:
    ESPNowComms espNow;
    LoraComms lora;
    CommMode activeMode;

    const uint8_t* espnowPeerMac;
    uint8_t loraPeerAddress;

    DataRecvCallback userRecvCallback;
    SendStatusCallback userSendCallback;

    // MODIFIÉ: Les callbacks internes reflètent les changements
    void onEspNowDataRecv(const uint8_t* mac, const uint8_t* data, int len);
    void onEspNowSendStatus(bool success);
    void onLoraDataRecv(const uint8_t* data, int len, uint8_t from);
};