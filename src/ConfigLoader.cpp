#include "ConfigLoader.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

bool parseMacAddress(const char* macStr, uint8_t* macArray) {
    if (sscanf(macStr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
               &macArray[0], &macArray[1], &macArray[2], 
               &macArray[3], &macArray[4], &macArray[5]) == 6) {
        return true;
    }
    return false;
}

bool loadConfig(Config& config) {
   
    File configFile = SPIFFS.open("/config.json", "r");
    if (!configFile) {
        Serial.println("Échec: Fichier /config.json introuvable sur SPIFFS.");
        return false;
    }


    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close(); 

    if (error) {
        Serial.print("Échec du parsing JSON: ");
        Serial.println(error.c_str());
        return false;
    }

  
    
    config.identity.farmId = doc["identity"]["farmId"] | "default_farm";
    config.identity.zoneId = doc["identity"]["zoneId"] | "default_zone";
    config.identity.nodeId = doc["identity"]["nodeId"] | "default_node";
    config.identity.isMaster = doc["identity"]["isMaster"];


    config.pins.led = doc["pins"]["led"];
    config.pins.led_brightness = doc["pins"]["led_brightness"] | 30;
    config.pins.soil_sensor = doc["pins"]["soil_sensor"];
    config.pins.soil_power = doc["pins"]["soil_power"];
    config.pins.dht11 = doc["pins"]["dht11"];
    config.pins.lora_m0 = doc["pins"]["lora_m0"];
    config.pins.lora_m1 = doc["pins"]["lora_m1"];
    config.pins.lora_aux = doc["pins"]["lora_aux"];
    config.pins.lora_rx = doc["pins"]["lora_rx"];
    config.pins.lora_tx = doc["pins"]["lora_tx"];


    config.calibration.soil_dry = doc["calibration"]["soil_dry"];
    config.calibration.soil_wet = doc["calibration"]["soil_wet"];

   
    config.network.master_mac_str = doc["network"]["master_mac"] | "00:00:00:00:00:00";
    config.network.enableESPNow = doc["network"]["enableESPNow"] | true;  
    config.network.enableLora = doc["network"]["enableLora"] | true;    
    config.network.lora_node_addr = doc["network"]["lora_node_addr"] | 1;
    config.network.lora_peer_addr = doc["network"]["lora_peer_addr"] | 2;
    config.network.lora_channel = doc["network"]["lora_channel"] | 23;

  
    config.logic.humidity_threshold = doc["logic"]["humidity_threshold"];

    


    if (!parseMacAddress(config.network.master_mac_str.c_str(), config.network.master_mac_bytes)) {
        Serial.println("Échec: Format MAC invalide dans config.json");
        return false;
    }

    Serial.println("Configuration chargée avec succès.");
    Serial.print("Node ID: ");
    Serial.println(config.identity.nodeId);
    Serial.print("Rôle: ");
    Serial.println(config.identity.isMaster ? "Master" : "Follower");

    return true;
}