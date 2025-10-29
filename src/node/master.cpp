#include "master.h"
#include <Arduino.h>

Master::Master(const Config& config)
    : config(config),
      actuator(config.pins.led, config.pins.led_brightness, config.logic.humidity_threshold),
      comms(config.network.lora_master_addr), // Adresse LoRa de cet appareil
      lastReceivedHumidity(0.0f) 
{}

void Master::begin() {
    actuator.begin();
    
    // Démarre le CommManager avec les configs
    comms.begin(config.network, config.pins, config.identity.isMaster);
    
    // S'abonne aux réceptions
    comms.registerRecvCallback([this](const SenderInfo& sender, const uint8_t* data, int len) {
        this->onDataReceived(sender, data, len);
    });
    
    Serial.print("Master démarré. Mode Comms: ");
    Serial.println(comms.getActiveMode() == CommMode::ESP_NOW ? "ESP-NOW" : "LORA");
}

void Master::update() {
    actuator.update();
    
    // Affiche le statut (Rouge/Vert) basé sur la dernière donnée reçue
    actuator.showStatus(lastReceivedHumidity);
}

// onDataReceived est INCHANGÉ (il fonctionnait déjà parfaitement)
void Master::onDataReceived(const SenderInfo& sender, const uint8_t* data, int len) {
    
    DeserializationError error = deserializeJson(jsonDoc, (const char*)data, len);

    if (error) {
        Serial.print("Erreur de parsing JSON ! ");
        Serial.println(error.c_str());
        return;
    }

    float humidity = jsonDoc["sensors"]["soilHumidity"];
    float temp = jsonDoc["sensors"]["temp"];
    const char* nodeId = jsonDoc["identity"]["nodeId"];

    Serial.print("Données reçues de ");
    Serial.print(nodeId);
    Serial.print(": ");
    Serial.print(humidity);
    Serial.print("% Humidité, ");
    Serial.print(temp);
    Serial.print("°C Temp. ");

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

    lastReceivedHumidity = humidity;
    actuator.showConnected(); 
}