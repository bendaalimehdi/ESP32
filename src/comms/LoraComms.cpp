#include "LoraComms.h"
#include <Arduino.h>

LoraComms::LoraComms(HardwareSerial& serial, Actuator* actuator)
    : loraSerial(serial),
      actuator(actuator),
      localId(0),
      defaultPeer(0),
      isAwake(false),
      onDataReceived(nullptr),
      onSendComplete(nullptr),
      rxIndex(0),
      awaitingAck(false),
      sendTime(0)
{
}

bool LoraComms::begin(const ConfigPins& pinConfig, const ConfigNetwork& netConfig) {
    this->pins       = pinConfig;
    this->localId    = netConfig.lora_node_addr;
    this->defaultPeer = netConfig.lora_peer_addr;

    Serial.println("[LoRa] Initialisation du DX-LR03.");

    pinMode(pins.lora_m1,  OUTPUT);
    pinMode(pins.lora_aux, INPUT_PULLUP);

    // RX / TX matériels définis dans ConfigPins
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

// -----------------------------------------------------------------------------
// Envoi haut niveau (avec attente d'ACK)
// -----------------------------------------------------------------------------
bool LoraComms::sendData(const uint8_t* data, int len) {
    return sendData(defaultPeer, data, len);
}

bool LoraComms::sendData(uint16_t destId, const uint8_t* data, int len) {
    // On garde la vérification ici pour les appels "normaux"
    if (len <= 0 || len > MAX_LORA_PAYLOAD - 4) { // <, srcId, len, >
        Serial.println("[LoRa] Payload trop long !");
        return false;
    }

    // Envoi brut (sans gestion d'ACK)
    if (!sendRawFrame(destId, data, len)) {
        return false;
    }

    // Ici seulement on arme la logique d'ACK
    awaitingAck = true;
    sendTime    = millis();

    Serial.print("[LoRa] --> TX OK (attend ACK) vers ");
    Serial.println(destId);

    return true;
}

// -----------------------------------------------------------------------------
// Envoi brut d'une trame LoRa (utilisé par sendData ET sendAck)
// -----------------------------------------------------------------------------
bool LoraComms::sendRawFrame(uint16_t destId, const uint8_t* data, int len) {
    if (len <= 0 || len > MAX_LORA_PAYLOAD - 4) { // sécurité interne
        Serial.println("[LoRa] (sendRawFrame) Payload trop long !");
        return false;
    }

    // Réveiller le module si besoin
    setMode(MODE_WAKE);
    if (!waitForAux(2000)) {
        Serial.println("[LoRa] ❌ Timeout AUX avant envoi !");
        return false;
    }

    // Indication visuelle TX
    if (actuator) {
        actuator->showLoraTxRx();
    }

    // --- 1. Construction de la trame logicielle [ '<' | srcId | len | data... | '>' ] ---
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
    loraSerial.write(0); // 3ème octet de l'en-tête (Channel / options)

    // --- 3. Envoi de la trame encapsulée ---
    loraSerial.write(payload_frame, payload_idx);
    loraSerial.flush();

    // Attendre que l'envoi RF soit terminé (AUX remonte)
    if (!waitForAux(5000)) {
        Serial.println("[LoRa] ❌ Timeout AUX après l'envoi physique.");
        return false;
    }

    // Log générique TX
    Serial.print("[LoRa] --> TX brut vers ");
    Serial.println(destId);

    return true;
}

// -----------------------------------------------------------------------------
// Boucle de gestion RX + Timeout ACK
// -----------------------------------------------------------------------------
void LoraComms::update() {
    // Gestion réception
    while (loraSerial.available()) {
        char c = loraSerial.read();
        
        // ACTUATOR : Afficher RX si c'est le début d'une nouvelle trame
        if (c == '<' && actuator) {
            actuator->showLoraTxRx();
        }
        
        processIncomingByte(c);
    }

    // Vérifier timeout ACK
    if (awaitingAck && millis() - sendTime > LORA_FRAME_TIMEOUT) {
        awaitingAck = false;
        if (onSendComplete) {
            onSendComplete(false);
        }
        Serial.println("[LoRa] ❌ Pas d’ACK: échec envoi");
    }
}

// -----------------------------------------------------------------------------
// Parsing de la trame entrée (framing '<' ... '>')
// -----------------------------------------------------------------------------
void LoraComms::processIncomingByte(char c) {
    if (c == '<') {
        rxIndex = 0; // Reset buffer
    } else if (c == '>') {
        // Fin de trame
        handleCompleteFrame();
        rxIndex = 0;
    } else if (rxIndex < MAX_LORA_PAYLOAD) {
        rxBuffer[rxIndex++] = (uint8_t)c;
    } else {
        // Débordement ou bruit -> reset
        rxIndex = 0;
    }
}

void LoraComms::handleCompleteFrame() {
    if (rxIndex < 2) return; 

    uint8_t srcId   = rxBuffer[0];
    uint8_t jsonLen = rxBuffer[1];

    if (jsonLen + 2 != rxIndex) { 
        Serial.println("[LoRa] ⚠️ Trame invalide (taille incohérente)");
        return;
    }

    const uint8_t* jsonData = &rxBuffer[2];

    // Traiter les ACK
    if (jsonLen == 1 && jsonData[0] == LORA_ACK_ID) {
        if (awaitingAck) {
            awaitingAck = false;
            if (onSendComplete) {
                onSendComplete(true);
            }
            Serial.println("[LoRa] ✔️ ACK reçu");
        }
        return;
    }

    // Sinon, c’est un message applicatif normal
    Serial.print("[LoRa] <-- Réception (");
    Serial.print(jsonLen);
    Serial.print(" octets) De: ");
    Serial.println(srcId);

    // Répondre automatiquement ACK (sans attente d'ACK pour l'ACK lui-même)
    sendAck(srcId);

    if (onDataReceived) {
        onDataReceived(jsonData, jsonLen, srcId);
    }
}

// -----------------------------------------------------------------------------
// Envoi d'un ACK (sans attente d'ACK)
// -----------------------------------------------------------------------------
bool LoraComms::sendAck(uint16_t destId) {
    uint8_t ackPayload = LORA_ACK_ID;
    bool ok = sendRawFrame(destId, &ackPayload, 1);

    if (ok) {
        Serial.print("[LoRa] --> ACK envoyé vers ");
        Serial.println(destId);
    } else {
        Serial.print("[LoRa] ❌ Échec envoi ACK vers ");
        Serial.println(destId);
    }

    return ok;
}

// -----------------------------------------------------------------------------
// Gestion des modes (M1 seulement)
// -----------------------------------------------------------------------------
void LoraComms::setMode(uint8_t mode) {
    bool newState = (mode == MODE_WAKE);
    if (newState == isAwake) return; 

    digitalWrite(pins.lora_m1, newState ? HIGH : LOW);
    isAwake = newState;
    delay(100); 
}

void LoraComms::sleep() {
    Serial.println("💤 DX-LR03 : Passage en mode veille (M1 LOW).");
    // (Optionnel : indiquer la veille via l'Actuator)
    // if (actuator) actuator->showSleep();
    setMode(MODE_SLEEP);
}

// -----------------------------------------------------------------------------
// Attente du signal AUX (module prêt)
// -----------------------------------------------------------------------------
bool LoraComms::waitForAux(unsigned long timeout) {
    unsigned long start = millis();
    // Dans ton montage, AUX est à LOW pendant l'activité, HIGH quand prêt
    while (digitalRead(pins.lora_aux) == LOW) {
        if (millis() - start > timeout) {
            return false;
        }
    }
    return true;
}
