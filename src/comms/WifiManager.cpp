#include "WifiManager.h"

// Initialisation du pointeur statique
WifiManager* WifiManager::_instance = nullptr;

WifiManager::WifiManager() 
    : _netConfig(nullptr),
      _mqttClient(_wifiClient),
      _lastReconnectAttempt(0),
      _commandCallback(nullptr) {
    _instance = this;
}

void WifiManager::begin(const ConfigNetwork& netConfig) {
    _netConfig = &netConfig;
    
    Serial.print("Connexion au Wi-Fi: ");
    Serial.println(_netConfig->wifi_ssid);
    WiFi.begin(_netConfig->wifi_ssid.c_str(), _netConfig->wifi_password.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        if (attempts++ > 20) {
            Serial.println("\nÉchec connexion Wi-Fi. Redémarrage...");
            delay(1000);
            ESP.restart();
        }
    }
    Serial.println("\nConnecté au Wi-Fi !");
    Serial.print("Adresse IP: ");
    Serial.println(WiFi.localIP());

    Serial.println("Initialisation du client NTP...");
    configTime(3600, 3600, "pool.ntp.org");
    
    _mqttClient.setServer(_netConfig->mqtt_broker.c_str(), _netConfig->mqtt_port);
    _mqttClient.setCallback(mqttCallback_static);
}

bool WifiManager::isTimeSynced() {
   
    return time(nullptr) > 1672531200; 
}

uint32_t WifiManager::getEpochTime() {
    return time(nullptr);
}

void WifiManager::registerCommandCallback(MqttCommandCallback cb) {
    _commandCallback = cb;
}

void WifiManager::update() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Wi-Fi perdu. Tentative de reconnexion...");
        WiFi.reconnect();
        _lastReconnectAttempt = millis(); // Empêche la reconnexion MQTT
        return;
    }
    
    if (!_mqttClient.connected()) {
        unsigned long now = millis();
        // Tente de se reconnecter toutes les 5 secondes
        if (now - _lastReconnectAttempt > 5000) {
            _lastReconnectAttempt = now;
            if (connectMqtt()) {
                _lastReconnectAttempt = 0; // Réussi
            }
        }
    } else {
        _mqttClient.loop();
    }
}

bool WifiManager::connectMqtt() {
    if (_netConfig == nullptr) return false;

    Serial.print("Tentative de connexion MQTT...");
    // Utilise l'ID du nœud (lu par ConfigLoader) comme ID client MQTT
    // Note: Le nodeId doit être chargé par le Master et passé
    // Pour l'instant, nous utilisons un ID statique
    
    if (_mqttClient.connect("esp32_master_gateway")) { // Vous pouvez passer g_config.identity.nodeId ici
        Serial.println("Connecté !");
        // S'abonne au topic de commandes
        _mqttClient.subscribe(_netConfig->topic_commands_down.c_str());
        Serial.print("Abonné à: ");
        Serial.println(_netConfig->topic_commands_down);
        return true;
    } else {
        Serial.print("Échec, rc=");
        Serial.print(_mqttClient.state());
        Serial.println(" Nouvelle tentative dans 5s");
        return false;
    }
}

bool WifiManager::publishTelemetry(const char* payload, int len) {
    if (!_mqttClient.connected()) {
        Serial.println("Erreur: MQTT déconnecté, télémétrie non envoyée.");
        return false;
    }
    
    if (_mqttClient.publish(_netConfig->topic_telemetry_up.c_str(), (const uint8_t*)payload, len)) {
        Serial.println("Télémétrie transférée au Pi via MQTT.");
        return true;
    }
    return false;
}

// Le callback statique C
void WifiManager::mqttCallback_static(char* topic, byte* payload, unsigned int length) {
    if (_instance && _instance->_commandCallback) {
        // Appelle la fonction C++
        _instance->_commandCallback(topic, payload, length);
    }
}