#include "CommManager.h"
#include <Arduino.h> // Pour Serial et Serial2

// Constructeur mis à jour pour initialiser lora(Serial2)
CommManager::CommManager() 
    : espNow(), 
      lora(Serial1), // Initialise le pilote LoRa Ebyte avec Serial2
      activeMode(CommMode::NONE),
      espnowPeerMac(nullptr),
      loraPeerAddress(0),
      userRecvCallback(nullptr),
      userSendCallback(nullptr) {}


bool CommManager::begin(const ConfigNetwork& netConfig, const ConfigPins& pinConfig, bool isMaster) {
    
    // 1. Stocker l'adresse du pair
    this->espnowPeerMac = netConfig.master_mac_bytes;
    this->loraPeerAddress = netConfig.lora_peer_addr; // Utilise l'adresse 16 bits du pair
    
    // --- 2. Tenter ESP-NOW (logique inchangée) ---
    if (netConfig.enableESPNow) {
        Serial.println("CommManager: Tentative d'initialisation d'ESP-NOW...");
        if (espNow.begin()) {
            Serial.println("CommManager: ESP-NOW initialisé avec succès.");
            if (this->espnowPeerMac != nullptr) {
                espNow.addPeer(this->espnowPeerMac);
            }
            
            espNow.registerRecvCallback([this](const uint8_t* mac, const uint8_t* d, int l){ this->onEspNowDataRecv(mac, d, l); });
            espNow.registerSendCallback([this](bool s){ this->onEspNowSendStatus(s); });
            
            this->activeMode = CommMode::ESP_NOW;
            return true;
        } else {
             Serial.println("CommManager: ESP-NOW activé, mais l'initialisation a échoué.");
        }
    } else {
        Serial.println("CommManager: ESP-NOW désactivé dans la configuration.");
    }

    // --- 3. Échec d'ESP-NOW, tenter LoRa ---
    if (netConfig.enableLora) { 
        Serial.println("CommManager: Tentative avec LoRa (Ebyte UART)...");
        
        // L'appel à begin() pour le pilote Ebyte est différent
        if (lora.begin(pinConfig, netConfig)) {
            Serial.println("CommManager: LoRa initialisé avec succès.");
            
            lora.registerRecvCallback([this](const uint8_t* d, int l, uint16_t f){ this->onLoraDataRecv(d, l, f); });
            
            this->activeMode = CommMode::LORA;
            return true;
        } else {
            Serial.println("CommManager: LoRa activé, mais l'initialisation a échoué.");
        }
    } else {
         Serial.println("CommManager: LoRa désactivé dans la configuration.");
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

bool CommManager::sendData(const char* jsonData) {
    const uint8_t* data = (const uint8_t*)jsonData;
    int len = strlen(jsonData); 
    
    if (len > MAX_PAYLOAD_SIZE) {
        Serial.println("Erreur: Payload JSON trop grand pour être envoyé !");
        return false;
    }

    switch(activeMode) {
        case CommMode::ESP_NOW:
            return espNow.sendData(espnowPeerMac, data, len);
        
        case CommMode::LORA: {
            // Utilise l'adresse 16 bits du pair
            bool success = lora.sendData(loraPeerAddress, data, len);
            if (userSendCallback) {
                userSendCallback(success);
            }
            return success; 
        }
        
        case CommMode::NONE:
        default:
            return false;
    }
}

// Ajoutez cette nouvelle fonction dans src/comms/CommManager.cpp

bool CommManager::sendDataToSender(const SenderInfo& recipient, const char* jsonData) {
    const uint8_t* data = (const uint8_t*)jsonData;
    int len = strlen(jsonData); 
    
    if (len > MAX_PAYLOAD_SIZE) {
        Serial.println("Erreur: Payload JSON trop grand pour être envoyé !");
        return false;
    }

    // Utilise le mode du destinataire, pas le mode "actif" global
    switch(recipient.mode) {
        case CommMode::ESP_NOW:
            if (recipient.macAddress == nullptr) return false;
            // Envoie à l'adresse MAC spécifique du destinataire
            return espNow.sendData(recipient.macAddress, data, len);
        
        case CommMode::LORA: {
            if (recipient.loraAddress == 0) return false; // Adresse LORA invalide
            
            // Envoie à l'adresse LORA 16 bits spécifique du destinataire
            bool success = lora.sendData(recipient.loraAddress, data, len);
            
            // Déclenche le callback (comme le fait l'envoi LoRa par défaut)
            if (userSendCallback) {
                userSendCallback(success);
            }
            return success; // Recommandé: retourner le statut réel de l'envoi
        }
        
        case CommMode::NONE:
        default:
            return false;
    }
}


void CommManager::onEspNowDataRecv(const uint8_t* mac, const uint8_t* data, int len) {
    if (userRecvCallback) {
        SenderInfo sender = {};
        sender.mode = CommMode::ESP_NOW;
        sender.macAddress = mac;
        sender.loraAddress = 0;
        userRecvCallback(sender, data, len); 
    }
}

void CommManager::onEspNowSendStatus(bool success) {
    if (userSendCallback) {
        userSendCallback(success);
    }
}

void CommManager::onLoraDataRecv(const uint8_t* data, int len, uint16_t from) {
    if (userRecvCallback) {
        SenderInfo sender = {};
        sender.mode = CommMode::LORA;
        sender.macAddress = nullptr;
        sender.loraAddress = from;
        userRecvCallback(sender, data, len); 
    }
}

void CommManager::update() {

    if (activeMode == CommMode::LORA) {
        lora.update();
    }
}