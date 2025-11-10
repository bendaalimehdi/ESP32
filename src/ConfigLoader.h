#pragma once
#include <Arduino.h>

#define MAX_PAYLOAD_SIZE 250
#define MAX_SOIL_SENSORS 5 // Définit une limite max de capteurs d'humidité
#define MAX_SEND_TIMES 10

// Structure pour les pins
struct ConfigPins {
    uint8_t led;
    uint8_t led_brightness;
   
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

// NOUVELLES STRUCTURES DE CAPTEURS
struct ConfigSensorSoil {
    bool enabled;
    uint8_t sensorPin;
    uint8_t powerPin;
    uint16_t dryValue;
    uint16_t wetValue;
};

struct ConfigSensorTemp {
    bool enabled;
    uint8_t pin;
};

struct ConfigSensors {
    ConfigSensorTemp temp_sensor;
    ConfigSensorSoil soil_sensors[MAX_SOIL_SENSORS];
    uint8_t num_soil_sensors; 
};
// FIN NOUVELLES STRUCTURES

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
    bool enableMqtt;
};

struct ConfigSendTime {
  uint8_t hour;
  uint8_t minute;
};

// Structure pour la logique
struct ConfigLogic {
    float humidity_thresholdMin;  
    float humidity_thresholdMax;    
    uint32_t defaultIrrigationDurationMs; //  Durée d'arrosage en ms
    ConfigSendTime send_times[MAX_SEND_TIMES];
    uint8_t num_send_times;
};



// --- La structure globale ---
struct Config {
    ConfigIdentity identity;
    ConfigPins pins;
    ConfigSensors sensors; 
    ConfigNetwork network;
    ConfigLogic logic;
  
};

/**
 * @brief 
 * @param config 
 * @return 
 */
bool loadConfig(Config& config);