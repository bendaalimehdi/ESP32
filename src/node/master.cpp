#include "master.h"
#include <Arduino.h>

Master::Master(const Config& config)
    : config(config),
      actuator(config.pins.led, config.pins.led_brightness, config.logic.humidity_threshold),
      wifi(),
      comms(), 
      valve1(config.pins.valve_1),
      valve2(config.pins.valve_2),
      valve3(config.pins.valve_3),
      valve4(config.pins.valve_4),
      valve5(config.pins.valve_5),
      lastReceivedHumidity(0.0f) 
{}

void Master::begin() {
    actuator.begin();
    valve1.begin();
    valve2.begin();
    valve3.begin();
    valve4.begin();
    valve5.begin();
    
    wifi.begin(config.network);
    comms.begin(config.network, config.pins, config.identity.isMaster);
    

    comms.registerRecvCallback([this](const SenderInfo& sender, const uint8_t* data, int len) {
        this->onDataReceived(sender, data, len);
    });

    wifi.registerCommandCallback([this](char* t, byte* p, unsigned int l) {
        this->onMqttCommandReceived(t, p, l);
    });
    
    Serial.print("Master démarré. Mode Comms: ");
    Serial.println(comms.getActiveMode() == CommMode::ESP_NOW ? "ESP-NOW" : "LORA");
}

void Master::update() {
    actuator.update();
    valve1.update();
    valve2.update();
    valve3.update();
    valve4.update();
    valve5.update();    
    
    wifi.update();
    actuator.showStatus(lastReceivedHumidity);
}


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

void Master::onMqttCommandReceived(char* topic, byte* payload, unsigned int length) {
    Serial.print("Commande MQTT reçue sur le topic: ");
    Serial.println(topic);

    // 1. Parser le JSON de commande
    StaticJsonDocument<256> cmdDoc;
    DeserializationError error = deserializeJson(cmdDoc, payload, length);

    if (error) {
        Serial.print("Erreur parsing JSON de commande ! ");
        Serial.println(error.c_str());
        return;
    }

    // 2. Interpréter la commande
    // Format attendu: {"valve": 1, "action": "open", "duration": 30000}
    
    int valve_num = cmdDoc["valve"];
    const char* action = cmdDoc["action"];
    uint32_t duration = cmdDoc["duration"] | 0; // 0 = infini

    if (action == nullptr || valve_num == 0) {
        Serial.println("Commande JSON invalide. 'valve' ou 'action' manquant.");
        return;
    }

    // 3. Agir sur la bonne vanne
    Electrovanne* targetValve = nullptr;
    if (valve_num == 1) targetValve = &valve1;
    if (valve_num == 2) targetValve = &valve2;
    if (valve_num == 3) targetValve = &valve3;
    if (valve_num == 4) targetValve = &valve4;
    if (valve_num == 5) targetValve = &valve5;

    if (targetValve == nullptr) {
        Serial.println("Numéro de vanne inconnu.");
        return;
    }

    // 4. Exécuter l'action
    if (strcmp(action, "open") == 0) {
        Serial.print("Commande: Ouverture Vanne ");
        Serial.print(valve_num);
        Serial.print(" pour ");
        Serial.print(duration);
        Serial.println("ms");
        targetValve->open(duration);
    } 
    else if (strcmp(action, "close") == 0) {
        Serial.print("Commande: Fermeture Vanne ");
        Serial.println(valve_num);
        targetValve->close();
    }
}

CommManager* Master::getCommManager() {
    return &comms; 
}