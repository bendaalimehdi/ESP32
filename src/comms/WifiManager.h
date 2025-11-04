#pragma once
#include <WiFi.h>
#include <PubSubClient.h>
#include <functional>
#include "ConfigLoader.h"

class WifiManager {
public:
    // Callback pour les commandes reçues (ex: du RPi)
    using MqttCommandCallback = std::function<void(char* topic, byte* payload, unsigned int length)>;

    WifiManager();
    
    /**
     * @brief Se connecte au Wi-Fi (bloquant)
     */
    void begin(const ConfigNetwork& netConfig);

    /**
     * @brief S'abonne au callback de commande
     */
    void registerCommandCallback(MqttCommandCallback cb);

    /**
     * @brief À appeler dans la loop() principale. Gère la boucle MQTT et la reconnexion.
     */
    void update();

    /**
     * @brief Publie les données de télémétrie (ex: vers le RPi)
     */
    bool publishTelemetry(const char* payload, int len);

private:
    const ConfigNetwork* _netConfig; // Pointeur vers la config
    String _nodeId; // ID du nœud (pour l'ID client MQTT)

    WiFiClient _wifiClient;
    PubSubClient _mqttClient;
    unsigned long _lastReconnectAttempt;
    MqttCommandCallback _commandCallback;

    bool connectMqtt();

    // Callback statique requis par PubSubClient
    static void mqttCallback_static(char* topic, byte* payload, unsigned int length);
    
    // Pointeur d'instance pour le callback statique
    static WifiManager* _instance;
};