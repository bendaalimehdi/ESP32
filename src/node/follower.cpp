#include "Follower.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <time.h> // Nécessaire pour localtime() et time()


Follower::Follower(const Config& config) 
    : config(config), 
      actuator(config.pins.led, config.pins.led_brightness),
      comms(&actuator),
      numSoilSensors(0), 
      tempSensor(nullptr),
      lastTimeCheck(0),
      alreadySentThisMinute(false), 
      timeIsSynced(false),
      lastCheckedMinute(-1), 
      syncFails(0), 
      lastSyncAttemptTime(0),
      isSending(false), 
      sendRetryCount(0) 
{
    // --- Initialisation dynamique des capteurs ---
    
    Serial.println("Initialisation dynamique des capteurs du Follower...");

    // 1. Capteurs d'humidité
    for (int i = 0; i < config.sensors.num_soil_sensors; i++) {
        const ConfigSensorSoil& s_cfg = config.sensors.soil_sensors[i];
        
        if (s_cfg.enabled) {
            Serial.print("Activation capteur humidité #");
            Serial.println(numSoilSensors);
            
          
            soilSensors[numSoilSensors] = new SoilHumiditySensor(
                s_cfg.sensorPin,
                s_cfg.powerPin,
                s_cfg.dryValue,
                s_cfg.wetValue
            );
            numSoilSensors++; // Incrémenter le nombre de capteurs actifs
        }
    }
    Serial.print(numSoilSensors);
    Serial.println(" capteurs d'humidité actifs.");

    // 2. Capteur de température
    if (config.sensors.temp_sensor.enabled) {
        Serial.println("Activation capteur de température.");
        tempSensor = new TemperatureSensor(config.sensors.temp_sensor.pin);
    } else {
        Serial.println("Capteur de température désactivé.");
    }
}

void Follower::begin() {
  
    
    // Initialiser tous les capteurs actifs
    for (int i = 0; i < numSoilSensors; i++) {
        soilSensors[i]->begin();
    }
    if (tempSensor) {
        tempSensor->begin();
    }
    
    comms.begin(config.network, config.pins, config.identity.isMaster);
    
    comms.registerSendCallback([this](bool success) {
        this->onDataSent(success);
    });

    comms.registerRecvCallback([this](const SenderInfo& sender, const uint8_t* data, int len) {
        this->onDataReceived(sender, data, len);
    });
    
    Serial.print("Follower démarré. Mode Comms: ");
    Serial.println(comms.getActiveMode() == CommMode::ESP_NOW ? "ESP-NOW" : "LORA");
}


void Follower::sendSensorData() {
    StaticJsonDocument<384> doc; 
    if (isSending) {
        Serial.println("⚠️ Envoi précédent toujours en cours, annulation.");
        return;
    }

    // 1. Identité
    doc["identity"]["farmId"] = config.identity.farmId;
    doc["identity"]["zoneId"] = config.identity.zoneId;
    doc["identity"]["nodeId"] = config.identity.nodeId;
    doc["identity"]["isMaster"] = config.identity.isMaster;
    
    // NOUVEAU: Ajouter un timestamp (0 si non synchronisé)
    doc["timestamp"] = timeIsSynced ? time(nullptr) : 0;
  
    // 2. Capteurs
    JsonObject sensorsObj = doc.createNestedObject("sensors");
        
    float firstHumidity = 0.0; // Pour la LED de statut

    for (int i = 0; i < numSoilSensors; i++) {
        float h = soilSensors[i]->read();
        String keyName = "soilHumidity" + String(i + 1); 
        sensorsObj[keyName] = round(h * 100.0) / 100.0;
    }

    // Ajouter la température 
    if (tempSensor) {
        float t = tempSensor->read();
        sensorsObj["temp"] = round(t * 100.0) / 100.0;
    } else {
        sensorsObj["temp"] = nullptr; 
    }
    
    char jsonString[384]; 
    serializeJson(doc, jsonString, sizeof(jsonString));
    lastJsonPayload = String(jsonString);
    
    Serial.print("Envoi JSON (Tentative 1/ ");
    Serial.print(MAX_SEND_RETRIES);
    Serial.println("): ");
    Serial.println(jsonString);

    isSending = true;
    sendRetryCount = 1;

    if (!comms.sendData(lastJsonPayload.c_str())) {
        Serial.println("Erreur d'envoi (file pleine?)");
    }
}

void Follower::update() {

    if (isSending) {
        return; 
    }

    // 1. Gérer la logique si l'heure n'est PAS synchronisée
    if (!timeIsSynced) {
        unsigned long now = millis();
        // Tenter d'envoyer (pour obtenir une synchro) toutes les 60 secondes
        if (now - lastSyncAttemptTime > 60000) { 
            if (syncFails < 3) {
                Serial.print("Heure non synchro. Tentative d'envoi #");
                Serial.println(syncFails + 1);
                sendSensorData(); // Envoie les données pour demander l'heure
                lastSyncAttemptTime = now;
                syncFails++;
            } else {
                // Après 3 échecs, on abandonne et on passe en mode fallback
                Serial.println("3 échecs de synchro. Passage en mode Fallback (envoi toutes les 5 min).");
                // On "prétend" que l'heure est synchro pour débloquer la logique,
                // mais time(nullptr) renverra 0 (ou une valeur proche).
                // L'ancien `if (epoch > 1672531200)` nous protège.
                // NOTE: On ne peut TOUJOURS pas utiliser le planning horaire.
                // On va juste envoyer toutes les 5 minutes
                sendSensorData();
                lastSyncAttemptTime = now; // Réutilise cette variable pour l'envoi fallback
            }
        }
        return; // Ne pas exécuter la logique basée sur l'heure
    }

    // 2. Si on arrive ici, timeIsSynced est TRUE
    // ... (Récupérer l'heure et la minute actuelles) ...
    time_t now_epoch = time(nullptr);
    struct tm* timeinfo = localtime(&now_epoch);
    int currentHour = timeinfo->tm_hour; 
    int currentMinute = timeinfo->tm_min;

    // ... (Détecter si la minute a changé) ...
    if (currentMinute != lastCheckedMinute) {
        alreadySentThisMinute = false;
        lastCheckedMinute = currentMinute;
    }

    // 4. Vérifier si on doit envoyer maintenant
    bool shouldSend = false;
    if (!alreadySentThisMinute) {
        for (int i = 0; i < config.logic.num_send_times; i++) {
            if (currentHour == config.logic.send_times[i].hour &&
                currentMinute == config.logic.send_times[i].minute) {
                
                shouldSend = true;
                break; 
            }
        }
    }
    
    // 5. Si on doit envoyer
    if (shouldSend) {
        alreadySentThisMinute = true; 
        
        Serial.print("Heure d'envoi configurée (");
        
        Serial.println(") atteinte. Envoi des données...");

        // --- Appel de la fonction refactorisée ---
        sendSensorData();
    }
}


void Follower::onDataSent(bool success) {
    if (!isSending) {
        // Rien à faire si aucun envoi n'est en cours
        return;
    }

    if (success) {
        Serial.println("Envoi OK");
        isSending = false;
        sendRetryCount = 0;
        return;
    }

    // Ici: échec d'envoi (pas de ACK)
    Serial.print("❌ Envoi Échoué (pas de ACK).");

    if (sendRetryCount < MAX_SEND_RETRIES) {
        sendRetryCount++;
        Serial.print(" Nouvel essai (");
        Serial.print(sendRetryCount);
        Serial.print("/");
        Serial.print(MAX_SEND_RETRIES);
        Serial.println(")...");

        // Attendre un petit peu avant de ré-essayer
        delay(100); // 100ms

        // Renvoyer le *dernier* payload
        if (!comms.sendData(lastJsonPayload.c_str())) {
            Serial.println("   Erreur ré-envoi (file pleine?)");
            // On arrête les envois pour ne pas rester bloqué
            isSending = false;
        }
        // Si l'envoi est accepté, on attend à nouveau le callback onDataSent
    } else {
        Serial.println(" Échec final après max de réessais.");
        isSending = false; // On a échoué, on abandonne
    }
}


CommManager* Follower::getCommManager() {
    return &comms; 
}

void Follower::onDataReceived(const SenderInfo& sender, const uint8_t* data, int len) {
    
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, (const char*)data, len);

    if (error) {
        Serial.println("Follower: Erreur JSON reçu.");
        return;
    }

    // Vérifier si c'est un message de synchro horaire
    const char* type = doc["type"];
    if (type != nullptr && strcmp(type, "timeSync") == 0) {
        
        uint32_t epoch = doc["epoch"];
        
        // Régler l'horloge interne de l'ESP32
        struct timeval tv;
        tv.tv_sec = epoch;
        tv.tv_usec = 0;
        settimeofday(&tv, nullptr); // Nécessite <sys/time.h>

        if (epoch > 1672531200) { 
            timeIsSynced = true;
            syncFails = 0;
            
            Serial.print("--- HEURE SYNCHRONISÉE ---");
            time_t now_epoch = time(nullptr);
            struct tm* timeinfo = localtime(&now_epoch);
            Serial.print(" Heure locale: ");
            Serial.print(asctime(timeinfo)); // 'asctime' ajoute un \n
        } else {
             Serial.println("Heure reçue invalide (epoch=0).");
        }
    }
}