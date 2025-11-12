#pragma once
#include <esp_now.h>
#include <WiFi.h>
#include <functional>
#include <esp_err.h>
#include "actuators/Actuator.h" 

class Actuator; // Déclaration anticipée

class ESPNowComms {
public:
    using DataRecvCallback  = std::function<void(const uint8_t* mac_addr, const uint8_t* data, int len)>;
    using SendStatusCallback = std::function<void(bool success)>;

    // MODIFIÉ : Constructeur qui prend Actuator*
    ESPNowComms(Actuator* actuator); 
  
    /**
     * @brief Initialise ESP-NOW
     * @param isMaster true si Master (Wi-Fi géré par WifiManager), false sinon (Follower)
     * @return true si succès, false sinon
     */
    bool begin(bool isMaster = false); // Renommé 'wifiAlreadyInited' en 'isMaster' pour plus de clarté

    void registerRecvCallback(DataRecvCallback cb);
    void registerSendCallback(SendStatusCallback cb);

    void addPeer(const uint8_t* mac_addr, uint8_t channel);
    bool isPeerExist(const uint8_t* mac_addr);
    
    bool sendData(const uint8_t* mac_addr, const uint8_t* data, int len);

private:
    static void onDataRecv_static(const uint8_t* mac, const uint8_t* data, int len);
    static void onDataSent_static(const uint8_t* mac, esp_now_send_status_t status);
    
    void handleDataRecv(const uint8_t* mac, const uint8_t* data, int len);
    void handleDataSent(const uint8_t* mac, esp_now_send_status_t status);

    static ESPNowComms* instance; 
    DataRecvCallback    onDataReceived;
    SendStatusCallback  onSendStatus;
    
    // AJOUTÉ : Pointeur vers l'Actuator
    Actuator* actuator; 
};