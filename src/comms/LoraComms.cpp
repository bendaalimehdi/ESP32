#include "LoraComms.h"
#include <Arduino.h>

#define MODE_NORMAL 0
#define MODE_WAKE 1
#define MODE_POWER_SAVE 2
#define MODE_CONFIG 3

// Buffer global
static String recvBuffer = "";

LoraComms::LoraComms(HardwareSerial& serial)
    : loraSerial(serial), onDataReceived(nullptr) {}

bool LoraComms::begin(const ConfigPins& pinConfig, const ConfigNetwork& netConfig) {
    this->pins = pinConfig;
    this->channel = netConfig.lora_channel;

    pinMode(pins.lora_m0, OUTPUT);
    pinMode(pins.lora_m1, OUTPUT);
    pinMode(pins.lora_aux, INPUT_PULLUP);

    loraSerial.begin(9600, SERIAL_8N1, pins.lora_rx, pins.lora_tx);

    Serial.println("=== Initialisation du module LoRa EBYTE ===");
    setMode(MODE_CONFIG);
    delay(100);

    if (!configureModule(netConfig)) {
        Serial.println("❌ ERREUR : Échec configuration module LoRa !");
        return false;
    }

    setMode(MODE_NORMAL);
    delay(100);
    while (loraSerial.available()) loraSerial.read();

    Serial.println("✅ Module LoRa EBYTE prêt (mode transmission fixe).");
    return true;
}

bool LoraComms::configureModule(const ConfigNetwork& netConfig) {
    uint8_t config_packet[6];

    config_packet[0] = 0xC0; // Sauvegarder à l'extinction
    config_packet[1] = (netConfig.lora_node_addr >> 8) & 0xFF; // Adresse haute
    config_packet[2] = netConfig.lora_node_addr & 0xFF;        // Adresse basse
    config_packet[3] = 0x1A; // UART 9600bps, 8N1, AirRate 2.4kbps
    config_packet[4] = netConfig.lora_channel; // Canal
    
    // --- CORRECTION ---
    // 0xC4 = Mode Transmission Fixe (Bit 7 = 1)
    // 0x44 était le Mode Transparent (Bit 7 = 0)
    config_packet[5] = 0xC4; 

    loraSerial.write(config_packet, 6);
    delay(50);

    if (loraSerial.available() && loraSerial.readBytes(config_packet, 6) == 6) {
        Serial.println("Ebyte : Configuration OK.");
        return true;
    }

    Serial.println("Ebyte : Pas de réponse à la configuration.");
    return false;
}

void LoraComms::registerRecvCallback(DataRecvCallback cb) {
    this->onDataReceived = cb;
}

// L'envoi est CORRECT (il envoie l'en-tête de 3 octets, 
// ce qui est nécessaire pour le Mode Transmission Fixe)
bool LoraComms::sendData(uint16_t destAddress, const uint8_t* data, int len) {
    setMode(MODE_NORMAL);

    if (!waitForAux(2000)) {
        Serial.println("Ebyte : Timeout AUX avant envoi");
        return false;
    }

    uint8_t dest_addr_h = (destAddress >> 8) & 0xFF;
    uint8_t dest_addr_l = destAddress & 0xFF;

    // Paquet : En-tête (3 octets) + Trame (<JSON>)
    loraSerial.write(dest_addr_h);
    loraSerial.write(dest_addr_l);
    loraSerial.write(this->channel);
    loraSerial.write('<');
    loraSerial.write(data, len);
    loraSerial.write('>');

    if (!waitForAux(5000)) {
        Serial.println("Ebyte : Timeout AUX pendant l'envoi.");
        return false;
    }

    // N'imprimez PAS ici, le Follower ne fait qu'envoyer.
    // Serial.println("📡 Données LoRa envoyées avec succès !");
    return true;
}

void LoraComms::update() {
    while (loraSerial.available()) {
        char c = (char)loraSerial.read();
        // Serial.print(c); // Décommentez SEULEMENT si vous débuggez

        // --- CORRECTION ---
        // En Mode Transmission Fixe (0xC4), le module de réception
        // ne transmet QUE LE PAYLOAD à l'ESP32.
        // L'en-tête de 3 octets (Adresse/Canal) est GÉRÉ et SUPPRIMÉ par le module.
        // Le buffer de l'ESP32 ne reçoit donc QUE "<JSON>".
        
        // Si le buffer est vide, on n'accepte que le début d'une trame
        if (recvBuffer.length() == 0 && c != '<') {
            continue; // Ignore le bruit avant le début
        }
        
        recvBuffer += c;

        // Détection de fin de trame
       if (c == '>') {
            int len = recvBuffer.length();
            Serial.print("\n✅ Trame complète reçue (");
            Serial.print(len);
            Serial.print(" octets): ");
            Serial.println(recvBuffer);

            // Trame valide = "<JSON>" (au moins 3 octets, ex: "<>")
            if (len >= 3 && onDataReceived) {
                const uint8_t* raw = (const uint8_t*)recvBuffer.c_str();
                
                // Pointeur vers le JSON (saute le '<' initial)
                uint8_t* jsonData = (uint8_t*)(raw + 1); 
                
                // Longueur du JSON (longueur totale - 2 pour '<' et '>')
                int jsonLen = len - 2; 

                onDataReceived(jsonData, jsonLen, 0); // 'from' est 0 (inconnu)
            
            } else if (len > 0) {
                Serial.println("⚠️ Trame corrompue reçue (ignorée).");
            }

            recvBuffer = ""; // reset après traitement
        }

        // Protection contre débordement
        if (recvBuffer.length() > (MAX_PAYLOAD_SIZE + 2)) { // +2 pour '<' et '>'
            Serial.println("⚠️ LoRa: Buffer overflow, reset.");
            recvBuffer = "";
        }
    }
}
// -------------------------------------------------------------
// GESTION DES MODES (CONFIG, NORMAL, etc.)
// -------------------------------------------------------------
void LoraComms::setMode(uint8_t mode) {
    digitalWrite(pins.lora_m0, (mode == MODE_WAKE || mode == MODE_CONFIG) ? HIGH : LOW);
    digitalWrite(pins.lora_m1, (mode == MODE_POWER_SAVE || mode == MODE_CONFIG) ? HIGH : LOW);
    delay(40);
}

// -------------------------------------------------------------
// ATTENTE DU SIGNAL AUX (module prêt)
// -------------------------------------------------------------
bool LoraComms::waitForAux(unsigned long timeout) {
    unsigned long start = millis();
    while (digitalRead(pins.lora_aux) == HIGH) {
        if (millis() - start > timeout) return false;
    }
    return true;
}
