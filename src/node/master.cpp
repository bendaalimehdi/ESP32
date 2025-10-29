#include "master.h"
#include <Arduino.h>
#include <ArduinoJson.h> // Ajouté

Master::Master()
    : actuator(PIN_NEOPIXEL),
      comms(LORA_MASTER_ADDR),
      lastReceivedHumidity(0.0f) {}

void Master::begin() {
    actuator.begin();
    
    comms.begin(nullptr, LORA_FOLLOWER_ADDR);
    
    // MODIFIÉ: La lambda accepte (sender, data, len)
    comms.registerRecvCallback([this](const SenderInfo& sender, const uint8_t* data, int len) {
        this->onDataReceived(sender, data, len);
    });
    
    Serial.print("Master démarré. Mode Comms: ");
    Serial.println(comms.getActiveMode() == CommMode::ESP_NOW ? "ESP-NOW" : "LORA");
}

void Master::update() {
    actuator.update();
    actuator.showStatus(lastReceivedHumidity);
}

// MODIFIÉ: Parse le JSON entrant
void Master::onDataReceived(const SenderInfo& sender, const uint8_t* data, int len) {
    
    // 1. Parser le JSON
    // Nous castons data (uint8_t*) en (const char*)
    DeserializationError error = deserializeJson(jsonDoc, (const char*)data, len);

    if (error) {
        Serial.print("Erreur de parsing JSON ! ");
        Serial.println(error.c_str());
        return;
    }

    // 2. Extraire les données
    float humidity = jsonDoc["sensors"]["soilHumidity"];
    float temp = jsonDoc["sensors"]["temp"];
    const char* nodeId = jsonDoc["identity"]["nodeId"]; // Ex: "NODE_001"

    // 3. Afficher les données
    Serial.print("Données reçues de ");
    Serial.print(nodeId);
    Serial.print(": ");
    Serial.print(humidity);
    Serial.print("% Humidité, ");
    Serial.print(temp);
    Serial.print("°C Temp. ");

    // Affiche l'expéditeur (couche transport)
    if (sender.mode == CommMode::ESP_NOW) {
        Serial.print(" via [ESP-NOW]: ");
        for (int i = 0; i < 6; i++) {
            if (sender.macAddress[i] < 0x10) Serial.print("0");
            Serial.print(sender.macAddress[i], HEX);
            if (i < 5) Serial.print(":");
        }
        Serial.println();
        
    } else if (sender.mode == CommMode::LORA) {
        Serial.print(" via [LORA]: 0x");
        Serial.println(sender.loraAddress, HEX);
    }

    // 4. Mettre à jour l'état
    lastReceivedHumidity = humidity;
    actuator.showConnected(); 
}