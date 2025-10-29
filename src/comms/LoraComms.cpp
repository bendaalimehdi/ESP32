#include "LoraComms.h"
#include <Arduino.h>
#include "Config.h" 

#define MAX_PAYLOAD_SIZE 250

LoraComms* LoraComms::instance = nullptr;

LoraComms::LoraComms(uint8_t localAddress) 
    : localAddress(localAddress), onDataReceived(nullptr) {
    instance = this;
}

bool LoraComms::begin(long frequency, const LoraPins& pins, uint8_t syncWord) {
    LoRa.setPins(pins.cs, pins.reset, pins.irq);
    if (!LoRa.begin(frequency)) {
        Serial.println("Erreur: Démarrage LoRa échoué !");
        return false;
    }
    LoRa.setSyncWord(syncWord);
    Serial.println("LoRa démarré avec succès !");
    LoRa.onReceive(onDataRecv_static);
    startReceiving();
    return true;
}

void LoraComms::registerRecvCallback(DataRecvCallback cb) {
    this->onDataReceived = cb;
}

// MODIFIÉ: Accepte data et len
bool LoraComms::sendData(uint8_t destAddress, const uint8_t* data, int len) {
    LoRa.beginPacket();
    LoRa.write(destAddress);
    LoRa.write(this->localAddress);
    LoRa.write(data, len); // Envoie le buffer brut
    return LoRa.endPacket(); 
}

void LoraComms::startReceiving() {
    LoRa.receive();
}

void LoraComms::onDataRecv_static(int packetSize) {
    if (instance) {
        instance->handleDataRecv(packetSize);
    }
}

// MODIFIÉ: Gère un payload de taille variable
void LoraComms::handleDataRecv(int packetSize) {
    // Un paquet doit contenir au moins les 2 octets d'en-tête
    if (packetSize <= 2) return;

    // int expectedSize = sizeof(MessageData) + 2; // SUPPRIMÉ
    // if (packetSize != expectedSize) { ... } // SUPPRIMÉ
    
    uint8_t destAddress = LoRa.read();
    uint8_t fromAddress = LoRa.read();
    
    // Calcule la taille réelle du payload (JSON)
    int payloadSize = packetSize - 2;

    if (destAddress != this->localAddress && destAddress != 0xFF) {
        Serial.print("Paquet LoRa ignoré (pour 0x");
        Serial.print(destAddress, HEX);
        Serial.println(")");
        while(LoRa.available()) LoRa.read();
        startReceiving();
        return; 
    }
    
  
    // Nous utilisons MAX_PAYLOAD_SIZE pour éviter un débordement
    uint8_t buffer[MAX_PAYLOAD_SIZE];
    
    if (payloadSize > MAX_PAYLOAD_SIZE) {
        Serial.println("Erreur: Paquet LoRa trop grand !");
        while(LoRa.available()) LoRa.read(); // Vide le buffer
        startReceiving();
        return;
    }

    // Lire le payload (la chaîne JSON)
    LoRa.readBytes(buffer, payloadSize);

    if (onDataReceived) {
        // MODIFIÉ: Passe le buffer brut et sa longueur
        onDataReceived(buffer, payloadSize, fromAddress);
    }
    
    startReceiving();
}