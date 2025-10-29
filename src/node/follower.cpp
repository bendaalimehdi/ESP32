#include "Follower.h"
#include <Arduino.h>
#include <ArduinoJson.h>

Follower::Follower(const Config& config) 
    : config(config), 
      actuator(config.pins.led, config.pins.led_brightness, config.logic.humidity_threshold), 
      sensor(config.pins.soil_sensor, config.pins.soil_power, 
             config.calibration.soil_dry, config.calibration.soil_wet), 
      tempSensor(config.pins.dht11),
      comms(config.network.lora_follower_addr),
      lastSendTime(0) 
{}

void Follower::begin() {
    actuator.begin();
    sensor.begin();
    tempSensor.begin();
    
    comms.begin(config.network, config.pins, config.identity.isMaster);
    
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
        
        float humidity = sensor.read();
        float temp = tempSensor.read(); 

      
        StaticJsonDocument<250> doc;

   
        doc["identity"]["farmId"] = config.identity.farmId;
        doc["identity"]["zoneId"] = config.identity.zoneId;
        doc["identity"]["nodeId"] = config.identity.nodeId;
        doc["identity"]["isMaster"] = config.identity.isMaster;
      

        doc["sensors"]["soilHumidity"] = round(humidity * 100.0) / 100.0;
        doc["sensors"]["temp"] = round(temp * 100.0) / 100.0;
        
        char jsonString[250]; 
        serializeJson(doc, jsonString, sizeof(jsonString));
        
        Serial.print("Envoi JSON: ");
        Serial.println(jsonString);

        actuator.showStatus(humidity);

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