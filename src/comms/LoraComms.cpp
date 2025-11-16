#include "LoraComms.h"
#include <Arduino.h>

// Définitions de mode simplifiées
#define MODE_SLEEP 0 
#define MODE_WAKE 1 

// Buffer global
static String recvBuffer = "";

LoraComms::LoraComms(HardwareSerial& serial, Actuator* actuator)
    : loraSerial(serial), rxIndex(0),
      onDataReceived(nullptr),
      onSendComplete(nullptr),
      awaitingAck(false),
      sendTime(0),
      localId(0),
      defaultPeer(0),
      isAwake(false),
      actuator(actuator) 
{
}

bool LoraComms::begin(const ConfigPins& pinConfig, const ConfigNetwork& netConfig) {
    this->pins = pinConfig;
    localId = netConfig.lora_node_addr;
    defaultPeer = netConfig.lora_peer_addr;

    Serial.println("[LoRa] Initialisation du DX-LR03...");

    pinMode(pins.lora_m1, OUTPUT);
    pinMode(pins.lora_aux, INPUT_PULLUP);

    loraSerial.begin(9600, SERIAL_8N1, pins.lora_rx, pins.lora_tx);

    setMode(MODE_WAKE);
    delay(300); 

    if (!waitForAux(2000)) {
        Serial.println("❌ DX-LR03 : Timeout AUX, module non prêt après l'allumage/réveil !");
        return false;
    }
    
    Serial.print("[LoRa] ✅ Prêt. Node ID: ");
    Serial.println(localId);
    return true;
}

void LoraComms::registerRecvCallback(DataRecvCallback cb) {
    onDataReceived = cb;
}

void LoraComms::registerSendCallback(SendCallback cb) {
    onSendComplete = cb;
}

bool LoraComms::sendData(const uint8_t* data, int len) {
    return sendData(defaultPeer, data, len);
}

bool LoraComms::sendData(uint16_t destId, const uint8_t* data, int len) {
    if (len > MAX_LORA_PAYLOAD - 3) {
        Serial.println("[LoRa] Payload trop long !");
        return false;
    }
    
    setMode(MODE_WAKE);
    if (!waitForAux(2000)) {
        Serial.println("[LoRa] ❌ Timeout AUX avant envoi !");
        return false;
    }

    // ➡️ ACTUATOR : Afficher TX (pour l'envoi de données)
    if (actuator) {
        actuator->showLoraTxRx(); 
    }

    // --- 1. Construction de la trame logicielle (avec ACK/Source ID) ---
    uint8_t payload_frame[MAX_LORA_PAYLOAD];
    int payload_idx = 0;

    payload_frame[payload_idx++] = '<';
    payload_frame[payload_idx++] = (uint8_t)localId; // srcId
    payload_frame[payload_idx++] = (uint8_t)len;     // payload len

    memcpy(&payload_frame[payload_idx], data, len);
    payload_idx += len;

    payload_frame[payload_idx++] = '>';
    
    // --- 2. Envoi de l'en-tête physique obligatoire DX-LR03 ---
    uint8_t dest_addr_h = (destId >> 8) & 0xFF;
    uint8_t dest_addr_l = destId & 0xFF;
    
    loraSerial.write(dest_addr_h);
    loraSerial.write(dest_addr_l);
    loraSerial.write(0); // 3ème octet de l'en-tête (Channel/Auxiliary Byte)
    
    // --- 3. Envoi de la trame encapsulée ---
    loraSerial.write(payload_frame, payload_idx);
    loraSerial.flush(); 

    // Attendre que l'envoi RF soit terminé (AUX redevient LOW)
    if (!waitForAux(5000)) {
        Serial.println("[LoRa] ❌ Timeout AUX après l'envoi physique.");
        return false;
    }

    Serial.print("[LoRa] --> TX OK (attend ACK) vers ");
    Serial.println(destId);
    
    awaitingAck = true;
    sendTime = millis();

    return true;
}

void LoraComms::update() {
    // Gestion réception
    while (loraSerial.available()) {
        char c = loraSerial.read();
        
        // ➡️ ACTUATOR : Afficher RX si c'est le début d'une nouvelle trame
        if (c == '<' && actuator) {
             actuator->showLoraTxRx();
        }
        
        processIncomingByte(c);
    }

    // Vérifier timeout ACK
    if (awaitingAck && millis() - sendTime > LORA_FRAME_TIMEOUT) {
        awaitingAck = false;
        if (onSendComplete) onSendComplete(false);
        Serial.println("[LoRa] ❌ Pas d’ACK: échec envoi");
    }
}

void LoraComms::processIncomingByte(char c) {
    if (c == '<') {
        rxIndex = 0; // Reset buffer
    } else if (c == '>') {
        // Fin de trame
        handleCompleteFrame();
        rxIndex = 0;
    } else if (rxIndex < MAX_LORA_PAYLOAD) {
        rxBuffer[rxIndex++] = c;
    } else {
        // Débordement ou bruit
        rxIndex = 0;
    }
}

void LoraComms::handleCompleteFrame() {
    if (rxIndex < 2) return; 

    uint8_t srcId = rxBuffer[0];
    uint8_t jsonLen = rxBuffer[1];

    if (jsonLen + 2 != rxIndex) { 
        Serial.println("[LoRa] ⚠️ Trame invalide (Taille incohérente)");
        return;
    }

    const uint8_t* jsonData = &rxBuffer[2];

    // Traiter les ACK
    if (jsonLen == 1 && jsonData[0] == LORA_ACK_ID) {
        if (awaitingAck) {
            awaitingAck = false;
            if (onSendComplete) onSendComplete(true);
            Serial.println("[LoRa] ✔️ ACK reçu");
        }
        return;
    }

    // Sinon, c’est un message normal
    Serial.print("[LoRa] <-- Réception (");
    Serial.print(jsonLen);
    Serial.print(" octets) De: ");
    Serial.println(srcId);

    // Répondre automatiquement ACK
    sendAck(srcId);

    if (onDataReceived) {
        onDataReceived(jsonData, jsonLen, srcId);
    }
}

bool LoraComms::sendAck(uint16_t destId) {
    // ➡️ ACTUATOR : Indiquer TX pour l'envoi de l'ACK
    if (actuator) {
        actuator->showLoraTxRx(); 
    }
    
    // --- 1. Construction de la trame logicielle ACK ---
    uint8_t ackPayload = LORA_ACK_ID;
    uint8_t payload_frame[5]; 
    int payload_idx = 0;

    payload_frame[payload_idx++] = '<';
    payload_frame[payload_idx++] = (uint8_t)localId;
    payload_frame[payload_idx++] = 1;  
    payload_frame[payload_idx++] = ackPayload;
    payload_frame[payload_idx++] = '>';
    
    // --- 2. Envoi de l'en-tête physique obligatoire DX-LR03 ---
    uint8_t dest_addr_h = (destId >> 8) & 0xFF;
    uint8_t dest_addr_l = destId & 0xFF;
    
    loraSerial.write(dest_addr_h);
    loraSerial.write(dest_addr_l);
    loraSerial.write(0); 
    
    // --- 3. Envoi de la trame encapsulée ---
    loraSerial.write(payload_frame, payload_idx);
    loraSerial.flush();
    
    Serial.print("[LoRa] --> ACK envoyé vers ");
    Serial.println(destId);
    return true;
}

// -------------------------------------------------------------
// GESTION DES MODES (M1 seulement)
// -------------------------------------------------------------
void LoraComms::setMode(uint8_t mode) {
    bool newState = (mode == MODE_WAKE);
    if (newState == isAwake) return; 

    digitalWrite(pins.lora_m1, newState ? HIGH : LOW);
    isAwake = newState;
    delay(100); 
}

void LoraComms::sleep() {
    Serial.println("💤 DX-LR03 : Passage en mode veille (M1 LOW).");
    if (actuator) {
        // Supposons une fonction pour indiquer la veille
        // actuator->showSleep(); 
    }
    setMode(MODE_SLEEP);
}

// -------------------------------------------------------------
// ATTENTE DU SIGNAL AUX (module prêt)
// -------------------------------------------------------------
bool LoraComms::waitForAux(unsigned long timeout) {
    unsigned long start = millis();
    while (digitalRead(pins.lora_aux) == LOW) {
        if (millis() - start > timeout) return false;
    }
    return true;
}