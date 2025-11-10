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


    StaticJsonDocument<1536> doc; // Augmentation de la taille pour plus de capteurs
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
    config.pins.lora_m0 = doc["pins"]["lora_m0"];
    config.pins.lora_m1 = doc["pins"]["lora_m1"];
    config.pins.lora_aux = doc["pins"]["lora_aux"];
    config.pins.lora_rx = doc["pins"]["lora_rx"];
    config.pins.lora_tx = doc["pins"]["lora_tx"];

    // NOUVEAU PARSING DES CAPTEURS
    JsonObject sensorsConfig = doc["sensors_config"];
    
    // Charger le capteur de température
    JsonObject tempConfig = sensorsConfig["temperature_sensor"];
    config.sensors.temp_sensor.enabled = tempConfig["enabled"] | false;
    config.sensors.temp_sensor.pin = tempConfig["pin"];

    // Charger les capteurs d'humidité
    JsonArray soilSensorsConfig = sensorsConfig["soil_humidity_sensors"];
    config.sensors.num_soil_sensors = 0;
    for (JsonObject s_cfg : soilSensorsConfig) {
        if (config.sensors.num_soil_sensors >= MAX_SOIL_SENSORS) {
            Serial.println("Avertissement: Nombre max de capteurs d'humidité atteint.");
            break;
        }

        ConfigSensorSoil& s = config.sensors.soil_sensors[config.sensors.num_soil_sensors];
        s.enabled = s_cfg["enabled"] | false;
        s.sensorPin = s_cfg["sensorPin"];
        s.powerPin = s_cfg["powerPin"];
        s.dryValue = s_cfg["dryValue"];
        s.wetValue = s_cfg["wetValue"];
        
        config.sensors.num_soil_sensors++;
    }
    // FIN NOUVEAU PARSING

 

    config.network.master_mac_str = doc["network"]["master_mac"] | "00:00:00:00:00:00";
    config.network.enableESPNow = doc["network"]["enableESPNow"] | true;  
    config.network.enableLora = doc["network"]["enableLora"] | true;    
    config.network.lora_node_addr = doc["network"]["lora_node_addr"] | 1;
    config.network.lora_peer_addr = doc["network"]["lora_peer_addr"] | 2;
    config.network.lora_channel = doc["network"]["lora_channel"] | 23;

    config.network.wifi_ssid = doc["network"]["wifi_ssid"] | "";
    config.network.wifi_password = doc["network"]["wifi_password"] | "";
    config.network.mqtt_broker = doc["network"]["mqtt_broker"] | "";
    config.network.mqtt_port = doc["network"]["mqtt_port"] | 1883;
    config.network.mqtt_user = doc["network"]["mqtt_user"] | "";
    config.network.mqtt_pass = doc["network"]["mqtt_pass"] | "";
    config.network.topic_telemetry_up = doc["network"]["topic_telemetry_up"] | "farm/telemetry";
    config.network.topic_commands_down = doc["network"]["topic_commands_down"] | "farm/commands/master/set";

  
    config.logic.humidity_thresholdMin = doc["logic"]["humidity_thresholdMin"] | 40.0;
    config.logic.humidity_thresholdMax = doc["logic"]["humidity_thresholdMax"] | 60.0;
    config.logic.defaultIrrigationDurationMs = doc["logic"]["defaultIrrigationDurationMs"] | 30000; // 30 sec par défaut
    config.logic.num_send_times = 0;
    JsonArray sendTimes = doc["logic"]["send_times"];
    if (!sendTimes.isNull()) {
        for (JsonObject t : sendTimes) {
            if (config.logic.num_send_times < MAX_SEND_TIMES) {
                
                config.logic.send_times[config.logic.num_send_times].hour = t["hour"].as<uint8_t>();
                config.logic.send_times[config.logic.num_send_times].minute = t["minute"].as<uint8_t>();
                
                Serial.print("Heure d'envoi chargée: ");
                Serial.print(config.logic.send_times[config.logic.num_send_times].hour);
                Serial.print(":");
                Serial.println(config.logic.send_times[config.logic.num_send_times].minute);

                config.logic.num_send_times++;
            }
        }
    }

    config.pins.valve_1 = doc["pins"]["valve_1"];
    config.pins.valve_2 = doc["pins"]["valve_2"];
    config.pins.valve_3 = doc["pins"]["valve_3"];
    config.pins.valve_4 = doc["pins"]["valve_4"];
    config.pins.valve_5 = doc["pins"]["valve_5"];

    
    if (!parseMacAddress(config.network.master_mac_str.c_str(), config.network.master_mac_bytes)) {
        Serial.println("Échec: Format MAC invalide dans config.json");
        return false;
    }

    Serial.println("Configuration chargée avec succès.");
    Serial.print("Node ID: ");
    Serial.println(config.identity.nodeId);
    Serial.print("Rôle: ");
    Serial.println(config.identity.isMaster ? "Master" : "Follower");
    Serial.print(config.sensors.num_soil_sensors);
    Serial.println(" capteurs d'humidité définis.");
    Serial.print("Capteur de température activé: ");
    Serial.println(config.sensors.temp_sensor.enabled ? "Oui" : "Non");

    return true;
}