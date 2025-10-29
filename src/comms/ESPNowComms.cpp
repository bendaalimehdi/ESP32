#include "ESPNowComms.h"
#include <Arduino.h>

ESPNowComms* ESPNowComms::instance = nullptr;

ESPNowComms::ESPNowComms() {
    instance = this; 
}

bool ESPNowComms::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    if (esp_now_init() != ESP_OK) {
        Serial.println("Erreur ESP-NOW Init");
        return false;
    }
    
    Serial.println("ESP-NOW Initialisé.");
    return true;
}

void ESPNowComms::registerRecvCallback(DataRecvCallback cb) {
    this->onDataReceived = cb;
    esp_now_register_recv_cb(onDataRecv_static);
}

void ESPNowComms::registerSendCallback(SendStatusCallback cb) {
    this->onSendStatus = cb;
    esp_now_register_send_cb(onDataSent_static);
}

void ESPNowComms::addPeer(const uint8_t* mac_addr) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, mac_addr, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
        Serial.println("Erreur ajout Peer");
    } else {
        Serial.println("Peer ajouté.");
    }
}

// MODIFIÉ: Accepte data et len
bool ESPNowComms::sendData(const uint8_t* mac_addr, const uint8_t* data, int len) {
    esp_err_t result = esp_now_send(mac_addr, data, len); // Utilise len
    return (result == ESP_OK);
}

// ----- Gestion des Callbacks -----

void ESPNowComms::onDataRecv_static(const uint8_t* mac, const uint8_t* data, int len) {
    if (instance) {
        instance->handleDataRecv(mac, data, len);
    }
}

void ESPNowComms::onDataSent_static(const uint8_t* mac, esp_now_send_status_t status) {
    if (instance) {
        instance->handleDataSent(mac, status);
    }
}

// MODIFIÉ: Ne désérialise plus, passe le buffer brut au callback
void ESPNowComms::handleDataRecv(const uint8_t* mac, const uint8_t* incomingData, int len) {
    // if (len == sizeof(MessageData)) { // SUPPRIMÉ
    //     MessageData data; // SUPPRIMÉ
    //     memcpy(&data, incomingData, sizeof(data)); // SUPPRIMÉ
        
        if (onDataReceived) {
            // MODIFIÉ: Passe le buffer brut et sa longueur
            onDataReceived(mac, incomingData, len);
        }
    // } // SUPPRIMÉ
}

void ESPNowComms::handleDataSent(const uint8_t* mac, esp_now_send_status_t status) {
    if (onSendStatus) {
        onSendStatus(status == ESP_NOW_SEND_SUCCESS);
    }
}