#pragma once
#include <Adafruit_NeoPixel.h> 

enum class LedState {
    IDLE,
    SEARCHING,      // ESP-NOW recherche (bleu clignotant)
    ESP_CONNECTING, // ESP-NOW connexion (bleu rapide)
    CONNECTED,      // ESP-NOW connecté (vert long)
    LORA_TXRX       // LoRa envoi/reçu (rouge rapide)
};

class Actuator {
public:
    // Constructeur simplifié (humidity_threshold n'était pas utilisé)
    Actuator(uint8_t pin, uint8_t brightness);
    
    void begin();
    void update(); 

    // --- Fonctions de contrôle ---
    void showSearching();    // Bleu clignotant
    void showEspNowConnecting(); // Bleu rapide
    void showConnected();    // Vert long
    void showLoraTxRx();     // Rouge rapide
    void showIdle();         // Éteint

private:
    uint8_t brightness; 
    
    LedState currentState;
    unsigned long stateStartTime;
    unsigned long lastBlinkTime;
    bool blinkState;

    Adafruit_NeoPixel pixel;
    
    void setColor(uint32_t color); 
};