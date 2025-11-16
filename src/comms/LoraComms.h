#pragma once
#include <HardwareSerial.h>
#include <functional>
#include "ConfigLoader.h"
#include "actuators/Actuator.h" 

// ⚠️ Constantes pour le protocole ACK
#define MAX_LORA_PAYLOAD 200 // Taille max de la trame UART incluant en-tête/pied
#define LORA_FRAME_TIMEOUT 5000 // Timeout pour l'ACK (5 secondes)
#define LORA_ACK_ID 0xFE // Identifiant pour l'ACK

// Modes du DX-LR03 basés sur M1
#define MODE_SLEEP 0 // M1 LOW : Entrer en mode veille
#define MODE_WAKE 1  // M1 HIGH : Réveiller le mode veille

class LoraComms {
public:
    using DataRecvCallback = std::function<void(const uint8_t* data, int len, uint16_t from)>;
    using SendCallback = std::function<void(bool success)>;

    // ⬅️ ACTUATOR AJOUTÉ AU CONSTRUCTEUR
    LoraComms(HardwareSerial& serial, Actuator* actuator);
    
    // Le begin utilise ConfigPins et ConfigNetwork pour l'ID et les broches
    bool begin(const ConfigPins& pins, const ConfigNetwork& netConfig);
    
    void registerRecvCallback(DataRecvCallback cb);
    void registerSendCallback(SendCallback cb);
    
    // Envoi au destinataire par défaut
    bool sendData(const uint8_t* data, int len);
    
    // Envoi à un destinataire spécifique (nécessite l'ID pour l'adressage physique)
    bool sendData(uint16_t destId, const uint8_t* data, int len);
    
    void update();
    
    // Fonction publique pour mettre le module en veille
    void sleep();

private:
    HardwareSerial& loraSerial;
    
    // ⬅️ ACTUATOR AJOUTÉ
    Actuator* actuator; 
    
    // Configuration de l'appareil
    ConfigPins pins;
    uint16_t localId;
    uint16_t defaultPeer;
    
    // Gestion de l'état
    bool isAwake;
    void setMode(uint8_t mode);
    bool waitForAux(unsigned long timeout = 1000); // DX-LR03 est prêt quand AUX est LOW
    
    // Gestion des callbacks
    DataRecvCallback onDataReceived;
    SendCallback onSendComplete;
    
    // Réception du protocole Frame [ < | srcId | len | json... | > ]
    uint8_t rxBuffer[MAX_LORA_PAYLOAD];
    int rxIndex;
    void processIncomingByte(char c);
    void handleCompleteFrame();

    // Gestion de l'ACK
    bool awaitingAck;
    unsigned long sendTime;
    
    // Fonctions d'assistance pour le protocole
    bool sendAck(uint16_t srcId);
};