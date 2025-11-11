#include "ESPNowComms.h"
#include <Arduino.h>

ESPNowComms* ESPNowComms::instance = nullptr;

ESPNowComms::ESPNowComms() {
    instance = this; 
    onDataReceived = nullptr;
    onSendStatus   = nullptr;
}

bool ESPNowComms::begin(bool wifiAlreadyInited) {
    if (!wifiAlreadyInited) {
        // Follower : on gère le Wi-Fi ici
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        Serial.println("ESPNowComms: Mode WIFI_STA activé (Follower).");
    } else {
        // Master : Wi-Fi déjà géré par WifiManager
        Serial.println("ESPNowComms: Wi-Fi déjà géré par le WifiManager (Master).");
    }

    esp_err_t initRes = esp_now_init();
    if (initRes != ESP_OK) {
        Serial.print("❌ Erreur ESP-NOW Init : ");
        Serial.print(initRes);
        Serial.print(" / ");
        Serial.println(esp_err_to_name(initRes));
        return false;
    }
    
    Serial.println("✅ ESP-NOW Initialisé.");
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

void ESPNowComms::addPeer(const uint8_t* mac_addr, uint8_t channel) {
    if (!mac_addr) {
        Serial.println("❌ ESP-NOW addPeer: mac_addr = NULL");
        return;
    }

    if (esp_now_is_peer_exist(mac_addr)) {
        Serial.print("ℹ️ ESP-NOW: Peer déjà existant pour ");
        for (int i = 0; i < 6; i++) {
            if (i) Serial.print(":");
            if (mac_addr[i] < 0x10) Serial.print("0");
            Serial.print(mac_addr[i], HEX);
        }
        Serial.println();
        return;
    }

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, mac_addr, 6);
    peerInfo.channel = channel;      // 0 = canal WiFi courant
    peerInfo.encrypt = false;
    
    esp_err_t err = esp_now_add_peer(&peerInfo);
    if (err != ESP_OK) {
        Serial.print("❌ Erreur ajout Peer ESP-NOW (");
        Serial.print(err);
        Serial.print(" / ");
        Serial.print(esp_err_to_name(err));
        Serial.println(")");
    } else {
        Serial.print("✅ Peer ESP-NOW ajouté : ");
        for (int i = 0; i < 6; i++) {
            if (i) Serial.print(":");
            if (mac_addr[i] < 0x10) Serial.print("0");
            Serial.print(mac_addr[i], HEX);
        }
        Serial.println();
    }
}

bool ESPNowComms::isPeerExist(const uint8_t* mac_addr) {
    if (!mac_addr) return false;
    return esp_now_is_peer_exist(mac_addr);
}

bool ESPNowComms::sendData(const uint8_t* mac_addr, const uint8_t* data, int len) {
    if (!mac_addr) {
        Serial.println("❌ ESP-NOW sendData: mac_addr = NULL");
        return false;
    }

    esp_err_t result = esp_now_send(mac_addr, data, len); 

    if (result != ESP_OK) {
        Serial.print("❌ ESP-NOW send error vers ");
        for (int i = 0; i < 6; i++) {
            if (i) Serial.print(":");
            if (mac_addr[i] < 0x10) Serial.print("0");
            Serial.print(mac_addr[i], HEX);
        }
        Serial.print(" -> code=");
        Serial.print(result);
        Serial.print(" / ");
        Serial.println(esp_err_to_name(result));
        return false;
    }

    // Le résultat ESP_OK signifie que le paquet est ENQUEUE.
    // Le succès réel est retourné plus tard dans onDataSent_static.
    return true;
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

void ESPNowComms::handleDataRecv(const uint8_t* mac, const uint8_t* incomingData, int len) {
    if (onDataReceived) {
        onDataReceived(mac, incomingData, len);
    }
}

void ESPNowComms::handleDataSent(const uint8_t* mac, esp_now_send_status_t status) {
    if (onSendStatus) {
        onSendStatus(status == ESP_NOW_SEND_SUCCESS);
    }
}
