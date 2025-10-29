#include "CommManager.h"
#include <Arduino.h> // Pour strlen

CommManager::CommManager(uint8_t loraLocalAddr) 
    : espNow(), 
      lora(loraLocalAddr),
      activeMode(CommMode::NONE),
      espnowPeerMac(nullptr),
      loraPeerAddress(0),
      userRecvCallback(nullptr),
      userSendCallback(nullptr) {}

// MODIFIÉ: Implémentation de la nouvelle signature begin()
bool CommManager::begin(const ConfigNetwork& netConfig, const ConfigPins& pinConfig, bool isMaster) {
    
    // 1. Stocker les adresses des pairs
    this->espnowPeerMac = netConfig.master_mac_bytes;
    if (isMaster) {
        // Si je suis le Master, mon pair est le Follower
        this->loraPeerAddress = netConfig.lora_follower_addr;
    } else {
        // Si je suis le Follower, mon pair est le Master
        this->loraPeerAddress = netConfig.lora_master_addr;
    }

    // --- 2. Tenter ESP-NOW ---
    if (espNow.begin()) {
        Serial.println("CommManager: ESP-NOW initialisé avec succès.");
        // (La correction pour éviter le crash sur nullptr est toujours là)
        if (this->espnowPeerMac != nullptr) {
            espNow.addPeer(this->espnowPeerMac);
        }
        
        espNow.registerRecvCallback([this](const uint8_t* mac, const uint8_t* d, int l){ this->onEspNowDataRecv(mac, d, l); });
        espNow.registerSendCallback([this](bool s){ this->onEspNowSendStatus(s); });
        
        this->activeMode = CommMode::ESP_NOW;
        return true;
    }

    // --- 3. Échec d'ESP-NOW, tenter LoRa ---
    Serial.println("CommManager: ESP-NOW a échoué. Tentative avec LoRa...");
    
    // Créer la structure LoraPins à partir de ConfigPins
    LoraPins loraPins = {
        .cs = pinConfig.lora_cs,
        .reset = pinConfig.lora_reset,
        .irq = pinConfig.lora_irq
    };
    
    // Utilise les valeurs de la config
    if (lora.begin(netConfig.lora_freq, loraPins, netConfig.lora_sync_word)) {
        Serial.println("CommManager: LoRa initialisé avec succès.");
        
        lora.registerRecvCallback([this](const uint8_t* d, int l, uint8_t f){ this->onLoraDataRecv(d, l, f); });
        
        this->activeMode = CommMode::LORA;
        return true;
    }

    // --- 4. Échec des deux ---
    Serial.println("CommManager: Échec de l'initialisation des deux systèmes !");
    this->activeMode = CommMode::NONE;
    return false;
}

void CommManager::registerRecvCallback(DataRecvCallback cb) {
    this->userRecvCallback = cb;
}

void CommManager::registerSendCallback(SendStatusCallback cb) {
    this->userSendCallback = cb;
}

// MODIFIÉ: Accepte une chaîne JSON
bool CommManager::sendData(const char* jsonData) {
    // Convertit la chaîne en buffer brut et obtient sa longueur
    const uint8_t* data = (const uint8_t*)jsonData;
    int len = strlen(jsonData); 
    
    // Vérifie si le payload n'est pas trop grand
    if (len > MAX_PAYLOAD_SIZE) {
        Serial.println("Erreur: Payload JSON trop grand pour être envoyé !");
        return false;
    }

    switch(activeMode) {
        case CommMode::ESP_NOW:
            return espNow.sendData(espnowPeerMac, data, len);
        
        case CommMode::LORA: {
            bool success = lora.sendData(loraPeerAddress, data, len);
            if (userSendCallback) {
                userSendCallback(success);
            }
            return true; 
        }
        
        case CommMode::NONE:
        default:
            return false;
    }
}

// --- Fonctions de relai (interne) ---

// MODIFIÉ: Accepte (mac, data, len)
void CommManager::onEspNowDataRecv(const uint8_t* mac, const uint8_t* data, int len) {
    if (userRecvCallback) {
        SenderInfo sender = {};
        sender.mode = CommMode::ESP_NOW;
        sender.macAddress = mac;
        sender.loraAddress = 0;
        userRecvCallback(sender, data, len); // Transférer au Master
    }
}

void CommManager::onEspNowSendStatus(bool success) {
    if (userSendCallback) {
        userSendCallback(success);
    }
}

// MODIFIÉ: Accepte (data, len, from)
void CommManager::onLoraDataRecv(const uint8_t* data, int len, uint8_t from) {
    if (userRecvCallback) {
        SenderInfo sender = {};
        sender.mode = CommMode::LORA;
        sender.macAddress = nullptr;
        sender.loraAddress = from;
        userRecvCallback(sender, data, len); // Transférer au Master
    }
}