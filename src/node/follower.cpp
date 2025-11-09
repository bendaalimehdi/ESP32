#include "Follower.h"
#include <Arduino.h>
#include <ArduinoJson.h>


Follower::Follower(const Config& config) 
    : config(config), 
      actuator(config.pins.led, config.pins.led_brightness, config.logic.humidity_threshold), 
      comms(),
      numSoilSensors(0), 
      tempSensor(nullptr),
      lastTimeCheck(0),
      alreadySentThisHour(false), 
      timeIsSynced(false),
      lastCheckedHour(-1)
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
            numSoilSensors++; 
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

void Follower::update() {



    // 1. On ne peut rien faire si l'heure n'est pas synchronisée
    if (!timeIsSynced) {
        return; 
    }

    // 2. Récupérer l'heure actuelle
    time_t now_epoch = time(nullptr);
    struct tm* timeinfo = localtime(&now_epoch);
    int currentHour = timeinfo->tm_hour; // Heure actuelle (0-23)

    // 3. Détecter si l'heure a changé (ex: passage de 9h à 10h)
    if (currentHour != lastCheckedHour) {
        Serial.print("Nouvelle heure détectée : ");
        Serial.print(currentHour);
        Serial.println("h. Réautorisation d'envoi.");
        
        alreadySentThisHour = false; // C'est une nouvelle heure, on a le droit d'envoyer.
        lastCheckedHour = currentHour;
    }

    // 4. Vérifier si on doit envoyer maintenant
    bool shouldSend = false;
    if (!alreadySentThisHour) {
        // Parcourir les heures d'envoi configurées
        for (int i = 0; i < config.logic.num_send_hours; i++) {
            if (currentHour == config.logic.send_hours[i]) {
                shouldSend = true;
                break; // On a trouvé une heure correspondante
            }
        }
    }
    
    // 5. Si on doit envoyer (bonne heure ET pas déjà envoyé cette heure)
    if (shouldSend) {
        
        // On marque comme "envoyé" pour CETTE heure
        alreadySentThisHour = true; 
        
        Serial.print("Heure d'envoi configurée (");
        Serial.print(currentHour);
        Serial.println("h) atteinte. Envoi des données...");

        // --- Début de la logique d'envoi (copiée de l'ancienne fonction) ---
        StaticJsonDocument<384> doc; 

        // 1. Identité
        doc["identity"]["farmId"] = config.identity.farmId;
        doc["identity"]["zoneId"] = config.identity.zoneId;
        doc["identity"]["nodeId"] = config.identity.nodeId;
        doc["identity"]["isMaster"] = config.identity.isMaster;
      
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
        
        Serial.print("Envoi JSON: ");
        Serial.println(jsonString);

        if (!comms.sendData(jsonString)) {
            Serial.println("Erreur d'envoi (file pleine?)");
        }
        // --- Fin de la logique d'envoi ---
    }
}
void Follower::onDataSent(bool success) {
    if (success) {
        Serial.println("Envoi OK");
        
    } else {
        Serial.println("Envoi Échoué");
        
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

        timeIsSynced = true;
        
        Serial.print("--- HEURE SYNCHRONISÉE ---");
        // Afficher l'heure pour vérification
        time_t now_epoch = time(nullptr);
        struct tm* timeinfo = localtime(&now_epoch);
        Serial.print(" Heure locale: ");
        Serial.print(asctime(timeinfo));
    }
}