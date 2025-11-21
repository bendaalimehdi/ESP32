#include "master.h"
#include <Arduino.h>

Master::Master(const Config& config)
    : config(config),
      actuator(config.pins.led, config.pins.led_brightness),
      wifi(),
      comms(&actuator),
      lastReceivedHumidity(0.0f),
      irrigationManager(nullptr)
{
// Déclarer les pointeurs d'Electrovanne dans le Master.h, allouer ici:
    size_t configValveCount = config.num_electrovalves;
    size_t maxIter = (configValveCount < MAX_VALVES) ? configValveCount : MAX_VALVES;
    
    
    
    numValves = 0; // Réinitialisation par sécurité
    for (size_t i = 0; i < maxIter; ++i) { 
        const auto& valveConfig = config.electrovalves[i];
        if (valveConfig.enabled) {
            valveArray[i] = new Electrovanne(valveConfig.pin);
            numValves++;
        } else {
            valveArray[i] = nullptr;
        }
    }
    
    irrigationManager = new IrrigationManager(config.logic, valveArray);
}

void Master::begin() {
    Serial.println("=== INITIALISATION MASTER ===");
    Serial.print("L'ADRESSE MAC DE CE MASTER EST : "); 
    Serial.println(WiFi.macAddress());
    
    actuator.begin();
    actuator.showSearching(); // Démarre en mode recherche
    
    for (size_t i = 0; i < numValves; ++i) {
            if (valveArray[i] != nullptr) {
                valveArray[i]->begin();
            }
        }
    
    // Démarrer le Wi-Fi AVANT ESP-NOW (pour le Master)
    //wifi.begin(config.network);

    lastWifiAttempt = millis() - WIFI_RETRY_INTERVAL_MS;
    wifiReady = false;
    
    // Initialiser les communications (ESP-NOW utilisera le Wi-Fi déjà initialisé)
    if (!comms.begin(config.network, config.pins, config.identity.isMaster)) {
        Serial.println("❌ ERREUR CRITIQUE: Échec initialisation communications !");
        actuator.showIdle();
        while(1) delay(100);
    }

    comms.registerRecvCallback([this](const SenderInfo& sender, const uint8_t* data, int len) {
        this->onDataReceived(sender, data, len);
    });

    wifi.registerCommandCallback([this](char* t, byte* p, unsigned int l) {
        this->onMqttCommandReceived(t, p, l);
    });
    
    Serial.print("✅ Master démarré. Mode Comms: ");
    if (comms.getActiveMode() == CommMode::ESP_NOW) {
        Serial.println("ESP-NOW");
        actuator.showConnected(); 
    } else if (comms.getActiveMode() == CommMode::LORA) {
        Serial.println("LORA");
    } else {
        Serial.println("AUCUN (ERREUR)");
    }
    Serial.println("===============================");
}

void Master::update() {
    actuator.update();
    for (size_t i = 0; i < numValves; ++i) {
            if (valveArray[i] != nullptr) {
                valveArray[i]->update();
            }
        }   
    
    //wifi.update();

        if (!wifiReady) {
        unsigned long now = millis();
        if (now - lastWifiAttempt >= WIFI_RETRY_INTERVAL_MS) {
            lastWifiAttempt = now;

            Serial.println("🔄 Tentative d'initialisation WiFi / MQTT...");

            wifi.begin(config.network);
            if (WiFi.status() == WL_CONNECTED) {
                wifiReady = true;
                Serial.println("✅ WiFi ok");
            } else {
                Serial.println("❌ Échec init WiFi. Nouvelle tentative dans 10 minutes.");
            }
        }
    } else {
        // Une fois initialisé, on laisse le WifiManager gérer les reconnexions internes
        wifi.update();
    }
    
    // Si MQTT reconnecté, vider la file d'attente
    if (wifi.isMqttConnected() && !telemetryQueue.empty()) {
        Serial.print("📤 MQTT reconnecté. Envoi de ");
        Serial.print(telemetryQueue.size());
        Serial.println(" messages en file d'attente...");

        for (const auto& payload : telemetryQueue) {
            wifi.publishTelemetry(payload.c_str(), payload.length());
            delay(100);
        }
        telemetryQueue.clear();
    }
}

void Master::onDataReceived(const SenderInfo& sender, const uint8_t* data, int len) {
    DeserializationError error = deserializeJson(jsonDoc, (const char*)data, len);

    if (error) {
        Serial.print("❌ Erreur de parsing JSON ! ");
        Serial.println(error.c_str());
        return;
    }

    // Extraire les données
    const char* nodeId = jsonDoc["identity"]["nodeId"];
    JsonObject sensorsObj = jsonDoc["sensors"]; 
    float temp = sensorsObj["temp"];

    Serial.print("✅ Données reçues de ");
    Serial.print(nodeId);
    Serial.print(": ");

    // Itérer sur toutes les humidités
    Serial.print("Humidités: [");

    for (int i = 1; i <= MAX_SOIL_SENSORS; i++) {
        String keyName = "soilHumidity" + String(i);
        
        if (sensorsObj.containsKey(keyName)) {
            float h = sensorsObj[keyName];
            Serial.print(keyName);
            Serial.print(": ");
            Serial.print(h, 2);
            Serial.print("%, ");
            // Logique de secours si MQTT déconnecté
            if (!wifi.isMqttConnected() || WiFi.status() != WL_CONNECTED) {
                Serial.println("⚠️ MQTT déconnecté, activation logique de secours.");
                if (irrigationManager) {
                    irrigationManager->processSensorData(i, h);
                }
            }
        }
    }
    
    Serial.print("], ");

    // Imprimer la température 
    if (!sensorsObj["temp"].isNull()) {
        Serial.print(temp, 2);
        Serial.print("°C ");
    } else {
        Serial.print("Temp: N/A ");
    }

    // --- TÉLÉMÉTRIE + FILE D'ATTENTE ---
    char telemetryPayload[MAX_PAYLOAD_SIZE];
    serializeJson(jsonDoc, telemetryPayload, sizeof(telemetryPayload));
    int payloadLen = strlen(telemetryPayload);

    if (wifi.isMqttConnected()) {
        if (wifi.publishTelemetry(telemetryPayload, payloadLen)) {
            Serial.println("→ 📡 Télémétrie transférée au serveur MQTT.");
        } else {
            Serial.println("→ ❌ Erreur envoi télémétrie (MQTT connecté).");
        }
    } else {
        Serial.println("→ 📦 MQTT déconnecté. Mise en file d'attente du message.");
        if (telemetryQueue.size() < MAX_QUEUE_SIZE) {
            telemetryQueue.push_back(std::string(telemetryPayload));
        } else {
            Serial.println("→ ⚠️ File d'attente pleine. Message de télémétrie perdu !");
        }
    }

    // Afficher le mode de communication
    if (sender.mode == CommMode::ESP_NOW) {
        Serial.print("via [ESP-NOW]: ");
        for (int i = 0; i < 6; i++) {
            if (sender.macAddress[i] < 0x10) Serial.print("0");
            Serial.print(sender.macAddress[i], HEX);
            if (i < 5) Serial.print(":");
        }
        Serial.println();
    } else if (sender.mode == CommMode::LORA) {
        Serial.print("via [LORA]: 0x");
        Serial.println(sender.loraAddress, HEX);
    }

    
    // --- SYNCHRO HORAIRE ---
    if (wifi.isTimeSynced()) {
        StaticJsonDocument<128> replyDoc;
        replyDoc["type"] = "timeSync";
        replyDoc["epoch"] = wifi.getEpochTime();
        
        char replyJson[128];
        serializeJson(replyDoc, replyJson);
        
        Serial.print("⏰ Envoi de la synchro horaire au Follower...");
        
        if (comms.sendDataToSender(sender, replyJson)) {
            Serial.println(" ✅ OK.");
        } else {
            Serial.println(" ❌ Échec.");
        }
    } else {
        Serial.println("⚠️ Heure du Master non synchronisée, pas de réponse.");
    }
}

void Master::onMqttCommandReceived(char* topic, byte* payload, unsigned int length) {
    Serial.print("📩 Commande MQTT reçue sur le topic: ");
    Serial.println(topic);

    StaticJsonDocument<256> cmdDoc;
    DeserializationError error = deserializeJson(cmdDoc, payload, length);

    if (error) {
        Serial.print("❌ Erreur parsing JSON de commande ! ");
        Serial.println(error.c_str());
        return;
    }

    // Format attendu: {"valve": 1, "action": "open", "duration": 30000}
    int valve_num = cmdDoc["valve"];
    const char* action = cmdDoc["action"];
    uint32_t duration = cmdDoc["duration"] | 0;

    if (action == nullptr || valve_num == 0) {
        Serial.println("❌ Commande JSON invalide. 'valve' ou 'action' manquant.");
        return;
    }

    // Sélectionner la bonne vanne
    Electrovanne* targetValve = nullptr;
    size_t index = valve_num - 1;

    if (index < numValves) {
        targetValve = valveArray[index];
    }

    if (targetValve == nullptr) {
        Serial.println("❌ Numéro de vanne inconnu.");
        return;
    }

    // Exécuter l'action
    if (strcmp(action, "open") == 0) {
        Serial.print("✅ Commande: Ouverture Vanne ");
        Serial.print(valve_num);
        Serial.print(" pour ");
        Serial.print(duration);
        Serial.println("ms");
        targetValve->open(duration);
    } 
    else if (strcmp(action, "close") == 0) {
        Serial.print("✅ Commande: Fermeture Vanne ");
        Serial.println(valve_num);
        targetValve->close();
    }
}

CommManager* Master::getCommManager() {
    return &comms; 
}