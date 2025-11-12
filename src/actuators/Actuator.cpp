#include "Actuator.h"

#define COLOR_RED     0xAA0000
#define COLOR_GREEN   0x00AA00
#define COLOR_BLUE    0x0000AA
#define COLOR_ORANGE  0xAA5500 // Nouveau : Orange
#define COLOR_OFF     0x000000

// Intervales de clignotement (en ms)
#define BLINK_FAST 100
#define BLINK_SLOW 250
#define BLINK_LONG 500

// Durée avant qu'un état temporaire ne s'arrête (en ms)
#define STATE_TIMEOUT_SHORT 2000  // 2 secondes
#define STATE_TIMEOUT_MEDIUM 5000 // 5 secondes


Actuator::Actuator(uint8_t pin, uint8_t brightness) 
    : brightness(brightness), 
      currentState(LedState::IDLE), // Démarrer éteint
      stateStartTime(0),
      lastBlinkTime(0),
      blinkState(false),
      pixel(1, pin, NEO_GRB + NEO_KHZ800) 
{}

void Actuator::begin() {
    pixel.begin(); 
    pixel.setBrightness(brightness); 
    pixel.show(); 
    stateStartTime = millis();
    showSearching(); // Démarrer en mode recherche
}

void Actuator::setColor(uint32_t color) {
    pixel.setPixelColor(0, color);
    pixel.show();
}

void Actuator::update() {
    unsigned long now = millis();

    switch (currentState) {
        
        case LedState::IDLE:
            // Ne rien faire, la LED est déjà éteinte
            break;

        case LedState::SEARCHING:
            // ESP-NOW Recherche/Initialisation: Bleu clignotant lent
            if (now - lastBlinkTime > BLINK_SLOW) {
                lastBlinkTime = now;
                blinkState = !blinkState;
                setColor(blinkState ? COLOR_BLUE : COLOR_OFF);
            }
            break;

        case LedState::ESP_CONNECTING:
            // ESP-NOW Connexion: Bleu clignotant rapide
            if (now - lastBlinkTime > BLINK_FAST) {
                lastBlinkTime = now;
                blinkState = !blinkState;
                setColor(blinkState ? COLOR_BLUE : COLOR_OFF);
            }
            // Après 5s, retourne au mode recherche
            if (now - stateStartTime > STATE_TIMEOUT_MEDIUM) {
                showSearching();
            }
            break;

        case LedState::CONNECTED:
            // ESP-NOW Connecté: Vert clignotant long
            if (now - lastBlinkTime > BLINK_LONG) {
                lastBlinkTime = now;
                blinkState = !blinkState;
                setColor(blinkState ? COLOR_GREEN : COLOR_OFF);
            }
            // Reste dans cet état (pas de timeout)
            break;

        case LedState::LORA_TXRX:
            // LoRa Envoi/Reçu: Rouge clignotant rapide
            if (now - lastBlinkTime > BLINK_FAST) {
                lastBlinkTime = now;
                blinkState = !blinkState;
                setColor(blinkState ? COLOR_RED : COLOR_OFF);
            }
            // Après 2s, retourne à l'état précédent (par défaut IDLE si le Follower n'est pas connecté)
            if (now - stateStartTime > STATE_TIMEOUT_SHORT) {
                showIdle(); 
            }
            break;
            
        case LedState::ESP_TXRX:
            // ESP-NOW Envoi/Reçu: Orange clignotant rapide
            if (now - lastBlinkTime > BLINK_FAST) {
                lastBlinkTime = now;
                blinkState = !blinkState;
                setColor(blinkState ? COLOR_ORANGE : COLOR_OFF);
            }
            // Après 2s, retourne à l'état précédent
            if (now - stateStartTime > STATE_TIMEOUT_SHORT) {
                showConnected(); // Le Master ou le Follower synchro est en CONNECTED
            }
            break;
    }
}

// --- Implémentation des fonctions publiques ---

void Actuator::showSearching() {
    if (currentState == LedState::SEARCHING) return;
    currentState = LedState::SEARCHING;
    stateStartTime = millis();
    blinkState = false; 
}

void Actuator::showEspNowConnecting() {
    currentState = LedState::ESP_CONNECTING;
    stateStartTime = millis();
    blinkState = false; 
}

void Actuator::showConnected() {
    if (currentState == LedState::CONNECTED) {
        stateStartTime = millis(); // Rafraîchit le timer si déjà connecté
        return;
    }
    currentState = LedState::CONNECTED;
    stateStartTime = millis();
    blinkState = false; 
    setColor(COLOR_GREEN); // S'allume direct
}

void Actuator::showLoraTxRx() {
    currentState = LedState::LORA_TXRX;
    stateStartTime = millis();
    blinkState = false; 
}

void Actuator::showEspNowTxRx() {
    currentState = LedState::ESP_TXRX;
    stateStartTime = millis();
    blinkState = false; 
}

void Actuator::showIdle() {
    currentState = LedState::IDLE;
    setColor(COLOR_OFF); // Éteint la LED
}