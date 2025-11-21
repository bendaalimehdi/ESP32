#pragma once
#include "comms/ESPNowComms.h"
#include "comms/LoraComms.h"
#include "ConfigLoader.h"
#include "actuators/Actuator.h" // Inclure Actuator.h

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
    using DataRecvCallback  = std::function<void(const SenderInfo& sender, const uint8_t* data, int len)>;
    using SendStatusCallback = std::function<void(bool success)>;

    void sleepIfLora();
    // AJOUTÉ : Constructeur prenant l'Actuator
    CommManager(Actuator* actuator); 
    

    bool begin(const ConfigNetwork& netConfig, const ConfigPins& pinConfig, bool isMaster);

    void registerRecvCallback(DataRecvCallback cb);
    void registerSendCallback(SendStatusCallback cb);

    bool sendData(const char* jsonData);
    bool sendDataToSender(const SenderInfo& recipient, const char* jsonData);

    void update();
    
    CommMode getActiveMode() const { return activeMode; }

    // Pour le Master : gestion dynamique des peers ESP-NOW
    void addEspNowPeer(const uint8_t* mac_addr);
    bool isEspNowPeerExist(const uint8_t* mac_addr);

private:
    // Membres gérés par le constructeur
    ESPNowComms espNow;
    LoraComms   lora;
    CommMode    activeMode;

    // Pointeur vers l'Actuator
    Actuator* actuator;

    const uint8_t* espnowPeerMac;  // côté Follower = MAC du Master, côté Master = nullptr
    uint16_t      loraPeerAddress; 
    uint8_t       espnowChannel;

    DataRecvCallback  userRecvCallback;
    SendStatusCallback userSendCallback;

    void onEspNowDataRecv(const uint8_t* mac, const uint8_t* data, int len);
    void onEspNowSendStatus(bool success);
    void onLoraDataRecv(const uint8_t* data, int len, uint16_t from); 
};