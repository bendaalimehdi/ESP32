#include "CommManager.h"
#include <Arduino.h>
#include <esp_wifi.h>

// MODIFIÉ : Le constructeur initialise espNow et lora avec l'Actuator
CommManager::CommManager(Actuator* actuator) 
    : espNow(actuator), 
      lora(Serial1, actuator), 
      activeMode(CommMode::NONE),
      espnowPeerMac(nullptr),
      loraPeerAddress(0),
      espnowChannel(1),
      userRecvCallback(nullptr),
      userSendCallback(nullptr),
      actuator(actuator) {}


bool CommManager::begin(const ConfigNetwork& netConfig, const ConfigPins& pinConfig, bool isMaster) {
    
    this->loraPeerAddress = netConfig.lora_peer_addr;

    // IMPORTANT :
    //  - Follower : peer fixe = MAC du Master (master_mac_bytes)
    //  - Master   : peer dynamique (nullptr ici)
    if (!isMaster) {
        this->espnowPeerMac = netConfig.master_mac_bytes;
    } else {
        this->espnowPeerMac = nullptr;
    }
    
    // --- TENTATIVE ESP-NOW ---
    if (netConfig.enableESPNow) {
        Serial.println("CommManager: Tentative d'initialisation d'ESP-NOW...");
        // isMaster est passé ici pour que ESPNowComms gère le Wi-Fi seulement si Follower
        if (espNow.begin(isMaster)) {
            Serial.println("CommManager: ESP-NOW initialisé avec succès.");
            // Configuration spécifique Follower
            if (!isMaster) {
                // Le Follower ajoute le Master comme peer
                if (this->espnowPeerMac != nullptr) {
                    Serial.print("Follower: Ajout du Master comme peer (MAC: ");
                    for (int i = 0; i < 6; i++) {
                        if (espnowPeerMac[i] < 0x10) Serial.print("0");
                        Serial.print(espnowPeerMac[i], HEX);
                        if (i < 5) Serial.print(":");
                    }
                    Serial.println(")");
                    
                    // 0 = canal WiFi courant
                    espNow.addPeer(this->espnowPeerMac, 0);
                } else {
                    Serial.println("⚠️ ERREUR : MAC du Master est NULL !");
                }
            } else {
                Serial.println("Master: les peers ESP-NOW seront ajoutés dynamiquement.");
            }
            
            // Enregistrer les callbacks
            espNow.registerRecvCallback(
                [this](const uint8_t* mac, const uint8_t* d, int l){ 
                    this->onEspNowDataRecv(mac, d, l); 
                }
            );
            espNow.registerSendCallback(
                [this](bool s){ 
                    this->onEspNowSendStatus(s); 
                }
            );
            
            this->activeMode = CommMode::ESP_NOW;
            Serial.println("✅ CommManager: ESP-NOW activé et prêt.");
            return true;
            
        } else {
            Serial.println("❌ CommManager: ESP-NOW activé, mais l'initialisation a échoué.");
        }
    } else {
        Serial.println("CommManager: ESP-NOW désactivé dans la configuration.");
    }

    // --- TENTATIVE LORA (FALLBACK) ---
    if (netConfig.enableLora) { 
        Serial.println("CommManager: Tentative avec LoRa ...");
        
        if (lora.begin(pinConfig, netConfig)) {
            Serial.println("✅ CommManager: LoRa initialisé avec succès.");
            
            lora.registerRecvCallback(
                [this](const uint8_t* d, int l, uint16_t f){ 
                    this->onLoraDataRecv(d, l, f); 
                }
            );
            
            this->activeMode = CommMode::LORA;
            return true;
            
        } else {
            Serial.println("❌ CommManager: LoRa activé, mais l'initialisation a échoué.");
        }
    } else {
        Serial.println("CommManager: LoRa désactivé dans la configuration.");
    }
    
    // --- ÉCHEC TOTAL ---
    Serial.println("❌ CommManager: ÉCHEC de l'initialisation des deux systèmes !");
    this->activeMode = CommMode::NONE;
    return false;
}

void CommManager::registerRecvCallback(DataRecvCallback cb) {
    this->userRecvCallback = cb;
}

void CommManager::registerSendCallback(SendStatusCallback cb) {
    this->userSendCallback = cb;
}

void CommManager::addEspNowPeer(const uint8_t* mac_addr) {
    if (!mac_addr) return;
    espNow.addPeer(mac_addr, 0); // 0 = canal WiFi courant
}

bool CommManager::isEspNowPeerExist(const uint8_t* mac_addr) {
    return espNow.isPeerExist(mac_addr);
}

bool CommManager::sendData(const char* jsonData) {
    const uint8_t* data = (const uint8_t*)jsonData;
    int len = strlen(jsonData); 
    
    if (len > MAX_PAYLOAD_SIZE) {
        Serial.println("❌ Erreur: Payload JSON trop grand pour être envoyé !");
        return false;
    }

    switch(activeMode) {
        case CommMode::ESP_NOW:
            Serial.print("📡 Envoi ESP-NOW (");
            Serial.print(len);
            Serial.println(" octets)...");
            if (!espnowPeerMac) {
                Serial.println("❌ ESP-NOW: Aucun peer configuré (espnowPeerMac = NULL).");
                return false;
            }
            return espNow.sendData(espnowPeerMac, data, len);
        
        case CommMode::LORA: {
            Serial.print("📡 Envoi LoRa vers 0x");
            Serial.print(loraPeerAddress, HEX);
            Serial.print(" (");
            Serial.print(len);
            Serial.println(" octets)...");
            
            bool success = lora.sendData(loraPeerAddress, data, len);
            if (userSendCallback) {
                userSendCallback(success);
            }
            return success; 
        }
        
        case CommMode::NONE:
        default:
            Serial.println("❌ Aucun mode de comm actif !");
            return false;
    }
}

bool CommManager::sendDataToSender(const SenderInfo& recipient, const char* jsonData) {
    const uint8_t* data = (const uint8_t*)jsonData;
    int len = strlen(jsonData); 
    
    if (len > MAX_PAYLOAD_SIZE) {
        Serial.println("❌ Erreur: Payload JSON trop grand pour être envoyé !");
        return false;
    }

    switch(recipient.mode) {
        case CommMode::ESP_NOW:
            if (recipient.macAddress == nullptr) {
                Serial.println("❌ Adresse MAC destinataire NULL !");
                return false;
            }
            // S'assurer que le peer existe (Master)
            if (!espNow.isPeerExist(recipient.macAddress)) {
                Serial.println("ℹ️ Ajout dynamique du peer ESP-NOW côté Master.");
                espNow.addPeer(recipient.macAddress, 0);
            }
            Serial.print("📡 Réponse ESP-NOW (");
            Serial.print(len);
            Serial.println(" octets)...");
            return espNow.sendData(recipient.macAddress, data, len);
        
        case CommMode::LORA: {
            if (recipient.loraAddress == 0) {
                Serial.println("❌ Adresse LoRa destinataire invalide !");
                return false;
            }
            
            Serial.print("📡 Réponse LoRa vers 0x");
            Serial.print(recipient.loraAddress, HEX);
            Serial.print(" (");
            Serial.print(len);
            Serial.println(" octets)...");
            
            bool success = lora.sendData(recipient.loraAddress, data, len);
            if (userSendCallback) {
                userSendCallback(success);
            }
            return success;
        }
        
        case CommMode::NONE:
        default:
            Serial.println("❌ Mode de comm destinataire invalide !");
            return false;
    }
}

void CommManager::onEspNowDataRecv(const uint8_t* mac, const uint8_t* data, int len) {
    Serial.print("✅ ESP-NOW: Réception de ");
    Serial.print(len);
    Serial.print(" octets depuis ");
    for (int i = 0; i < 6; i++) {
        if (mac[i] < 0x10) Serial.print("0");
        Serial.print(mac[i], HEX);
        if (i < 5) Serial.print(":");
    }
    Serial.println();
    
    if (userRecvCallback) {
        SenderInfo sender = {};
        sender.mode       = CommMode::ESP_NOW;
        sender.macAddress = mac;
        sender.loraAddress = 0;
        userRecvCallback(sender, data, len); 
    }
}

void CommManager::onEspNowSendStatus(bool success) {
    if (success) {
        Serial.println("✅ ESP-NOW: Envoi réussi !");
    } else {
        Serial.println("❌ ESP-NOW: Échec d'envoi !");
    }
    
    if (userSendCallback) {
        userSendCallback(success);
    }
}

void CommManager::onLoraDataRecv(const uint8_t* data, int len, uint16_t from) {
    Serial.print("✅ LoRa: Réception de ");
    Serial.print(len);
    Serial.print(" octets depuis 0x");
    Serial.println(from, HEX);
    
    if (userRecvCallback) {
        SenderInfo sender = {};
        sender.mode       = CommMode::LORA;
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