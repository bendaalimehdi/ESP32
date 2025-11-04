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
    uint8_t lora_m0;
    uint8_t lora_m1;
    uint8_t lora_aux;
    uint8_t lora_rx;
    uint8_t lora_tx;

    uint8_t valve_1;
    uint8_t valve_2;
    uint8_t valve_3;
    uint8_t valve_4;
    uint8_t valve_5;
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
    uint8_t master_mac_bytes[6]; 
    bool enableESPNow;          
    bool enableLora;            
    uint16_t lora_node_addr; 
    uint16_t lora_peer_addr;
    uint8_t  lora_channel;

    String wifi_ssid;
    String wifi_password;
    String mqtt_broker;
    uint16_t mqtt_port;
    String mqtt_user;
    String mqtt_pass;
    String topic_telemetry_up;
    String topic_commands_down;
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