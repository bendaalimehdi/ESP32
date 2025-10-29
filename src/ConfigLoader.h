#pragma once
#include <Arduino.h>

#define MAX_PAYLOAD_SIZE 250

// Structure pour les pins
struct ConfigPins {
    uint8_t led;
    uint8_t led_brightness;
    uint8_t soil_sensor;
    uint8_t soil_power;
    uint8_t dht11;
    uint8_t lora_cs;
    uint8_t lora_reset;
    uint8_t lora_irq;
};

// Structure pour l'identité
struct ConfigIdentity {
    String farmId;
    String zoneId;
    String nodeId;
    bool isMaster;
};

// Structure pour la calibration
struct ConfigCalibration {
    int soil_dry;
    int soil_wet;
};

// Structure pour le réseau
struct ConfigNetwork {
    String master_mac_str;
    uint8_t master_mac_bytes[6]; // L'adresse MAC parsée
    long lora_freq;
    uint8_t lora_sync_word;
    uint8_t lora_master_addr;
    uint8_t lora_follower_addr;
};

// Structure pour la logique
struct ConfigLogic {
    float humidity_threshold;
};

// --- La structure globale ---
struct Config {
    ConfigIdentity identity;
    ConfigPins pins;
    ConfigCalibration calibration;
    ConfigNetwork network;
    ConfigLogic logic;
  
};

/**
 * @brief 
 * @param config 
 * @return 
 */
bool loadConfig(Config& config);