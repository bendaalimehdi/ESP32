#include "Follower.h"
#include <Arduino.h>
#include <ArduinoJson.h> // Ajouté

Follower::Follower() 
    : actuator(PIN_NEOPIXEL),
      sensor(PIN_SOIL_SENSOR, PIN_SOIL_POWER, SENSOR_DRY_VALUE, SENSOR_WET_VALUE), 
      tempSensor(PIN_DHT22),
      comms(LORA_FOLLOWER_ADDR), 
      lastSendTime(0) 
      {}

void Follower::begin() {
    actuator.begin();
    sensor.begin();
    tempSensor.begin();
    
    comms.begin(masterMacAddress, LORA_MASTER_ADDR);
    
    comms.registerSendCallback([this](bool success) {
        this->onDataSent(success);
    });
    
    Serial.print("Follower démarré. Mode Comms: ");
    Serial.println(comms.getActiveMode() == CommMode::ESP_NOW ? "ESP-NOW" : "LORA");
}

void Follower::update() {
    actuator.update();
    
    unsigned long now = millis();
    
    if (now - lastSendTime > 5000) {
        lastSendTime = now;
        
        // 1. Lire les capteurs
        float humidity = sensor.read();
        float temp = tempSensor.read(); 

        // 2. Construire le document JSON
        StaticJsonDocument<MAX_PAYLOAD_SIZE> doc;

        // Remplir l'objet "identity" depuis Config.h
        doc["identity"]["farmId"] = FARM_ID;
        doc["identity"]["zoneId"] = ZONE_ID;
        doc["identity"]["nodeId"] = NODE_ID;
        doc["identity"]["isMaster"] = IS_MASTER_NODE;

        // Remplir l'objet "sensors"
        // Note : round() pour 2 chiffres après la virgule, propre
        doc["sensors"]["soilHumidity"] = round(humidity * 100.0) / 100.0;
        doc["sensors"]["temp"] = round(temp * 100.0) / 100.0;
        
        // 3. Sérialiser le JSON dans une chaîne
        char jsonString[MAX_PAYLOAD_SIZE];
        serializeJson(doc, jsonString, sizeof(jsonString));
        
        Serial.print("Envoi JSON: ");
        Serial.println(jsonString);

        // 4. Mettre à jour la LED locale
        actuator.showStatus(humidity);

        // 5. Envoyer la chaîne JSON
        if (!comms.sendData(jsonString)) {
            Serial.println("Erreur d'envoi (file pleine?)");
            actuator.showSearching(); 
        }
    }
}

void Follower::onDataSent(bool success) {
    if (success) {
        Serial.println("Envoi OK");
        actuator.showConnected();
    } else {
        Serial.println("Envoi Échoué");
        actuator.showSearching();
    }
}