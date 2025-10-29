#include "ConfigLoader.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

// Fonction privée pour parser "AA:BB:CC:DD:EE:FF" en un tableau d'octets
bool parseMacAddress(const char* macStr, uint8_t* macArray) {
    if (sscanf(macStr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
               &macArray[0], &macArray[1], &macArray[2], 
               &macArray[3], &macArray[4], &macArray[5]) == 6) {
        return true;
    }
    return false;
}

bool loadConfig(Config& config) {
    // 1. Ouvrir le fichier
    File configFile = SPIFFS.open("/config.json", "r");
    if (!configFile) {
        Serial.println("Échec: Fichier /config.json introuvable sur SPIFFS.");
        return false;
    }

    // 2. Parser le JSON
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close(); // Fermer le fichier dès que possible

    if (error) {
        Serial.print("Échec du parsing JSON: ");
        Serial.println(error.c_str());
        return false;
    }

    // 3. Remplir la structure Config
    
    // Identity
    config.identity.farmId = doc["identity"]["farmId"] | "default_farm";
    config.identity.zoneId = doc["identity"]["zoneId"] | "default_zone";
    config.identity.nodeId = doc["identity"]["nodeId"] | "default_node";
    config.identity.isMaster = doc["identity"]["isMaster"];

    // Pins
    config.pins.led = doc["pins"]["led"];
    config.pins.led_brightness = doc["pins"]["led_brightness"] | 30;
    config.pins.soil_sensor = doc["pins"]["soil_sensor"];
    config.pins.soil_power = doc["pins"]["soil_power"];
    config.pins.dht11 = doc["pins"]["dht11"];
    config.pins.lora_cs = doc["pins"]["lora_cs"];
    config.pins.lora_reset = doc["pins"]["lora_reset"];
    config.pins.lora_irq = doc["pins"]["lora_irq"];

    // Calibration
    config.calibration.soil_dry = doc["calibration"]["soil_dry"];
    config.calibration.soil_wet = doc["calibration"]["soil_wet"];

    // Network
    config.network.master_mac_str = doc["network"]["master_mac"] | "00:00:00:00:00:00";
    config.network.lora_freq = doc["network"]["lora_freq"];
    config.network.lora_sync_word = doc["network"]["lora_sync_word"];
    config.network.lora_master_addr = doc["network"]["lora_master_addr"];
    config.network.lora_follower_addr = doc["network"]["lora_follower_addr"];

    // Logic
    config.logic.humidity_threshold = doc["logic"]["humidity_threshold"];

    

    // 4. Post-traitement (Parser la MAC)
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