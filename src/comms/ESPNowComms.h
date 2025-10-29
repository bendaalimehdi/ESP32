#pragma once
#include <esp_now.h>
#include <WiFi.h>
#include <functional>
// #include "SharedStructures.h" // N'est plus nécessaire

class ESPNowComms {
public:
    // MODIFIÉ: Le callback envoie un buffer brut (data) et sa longueur (len)
    using DataRecvCallback = std::function<void(const uint8_t* mac_addr, const uint8_t* data, int len)>;
    using SendStatusCallback = std::function<void(bool success)>;

    ESPNowComms();
    bool begin(); 

    void registerRecvCallback(DataRecvCallback cb);
    void registerSendCallback(SendStatusCallback cb);

    void addPeer(const uint8_t* mac_addr);
    
    // MODIFIÉ: Envoie un buffer de données (data) d'une longueur (len)
    bool sendData(const uint8_t* mac_addr, const uint8_t* data, int len);

private:
    static void onDataRecv_static(const uint8_t* mac, const uint8_t* data, int len);
    static void onDataSent_static(const uint8_t* mac, esp_now_send_status_t status);
    
    void handleDataRecv(const uint8_t* mac, const uint8_t* data, int len);
    void handleDataSent(const uint8_t* mac, esp_now_send_status_t status);

    static ESPNowComms* instance; 
    DataRecvCallback onDataReceived;
    SendStatusCallback onSendStatus;
};