#include "Actuator.h"

// Définition des couleurs (Rouge, Vert, Bleu) pour NeoPixel
// Vous pouvez ajuster la luminosité de chaque couleur ici (ex: 0x550000 pour un rouge faible)
#define COLOR_RED     0xAA0000
#define COLOR_GREEN   0x00AA00
#define COLOR_BLUE    0x0000AA
#define COLOR_OFF     0x000000

// MODIFIÉ: Le constructeur initialise l'objet 'pixel' et stocke les configs
Actuator::Actuator(uint8_t pin, uint8_t brightness, float humidity_threshold) 
    : brightness(brightness), // Stocke la luminosité
      humidity_threshold(humidity_threshold), // Stocke le seuil
      currentState(LedState::SEARCHING),
      stateStartTime(0),
      lastBlinkTime(0),
      blinkState(false),
      pixel(1, pin, NEO_GRB + NEO_KHZ800) // Initialise l'objet NeoPixel
{}

void Actuator::begin() {
    pixel.begin(); // Démarre le pixel
    pixel.setBrightness(brightness); // Utilise la luminosité de la config
    pixel.show(); // Initialise le pixel (éteint)
    stateStartTime = millis();
}

// MODIFIÉ: Implémente la fonction pour NeoPixel
void Actuator::setColor(uint32_t color) {
    pixel.setPixelColor(0, color);
    pixel.show();
}

void Actuator::update() {
    unsigned long now = millis();

    switch (currentState) {
        case LedState::SEARCHING:
            // Clignotement bleu rapide
            if (now - lastBlinkTime > 100) {
                lastBlinkTime = now;
                blinkState = !blinkState;
                setColor(blinkState ? COLOR_BLUE : COLOR_OFF);
            }
            break;

        case LedState::CONNECTED:
            // Clignotement vert rapide
            if (now - lastBlinkTime > 100) {
                lastBlinkTime = now;
                blinkState = !blinkState;
                setColor(blinkState ? COLOR_GREEN : COLOR_OFF);
            }
            // Après 20 secondes, repasser en mode IDLE
            if (now - stateStartTime > 20000) {
                currentState = LedState::IDLE;
                // La couleur sera définie par le prochain appel à showStatus()
            }
            break;

        case LedState::IDLE:
            // Ne fait rien. La couleur est gérée par showStatus()
            break;
    }
}

void Actuator::showSearching() {
    currentState = LedState::SEARCHING;
    stateStartTime = millis();
}

void Actuator::showConnected() {
    if (currentState == LedState::CONNECTED) {
        stateStartTime = millis(); // Réinitialise le timer de 20s
        return;
    }
    currentState = LedState::CONNECTED;
    stateStartTime = millis();
    blinkState = false; 
}

void Actuator::showStatus(float humidity) {
    // Ce statut n'est affiché que si la LED est en mode IDLE
    if (currentState == LedState::IDLE) {
        // Utilise le seuil membre (stocké)
        if (humidity > humidity_threshold) {
            setColor(COLOR_GREEN); // Humide = Vert
        } else {
            setColor(COLOR_RED);   // Sec = Rouge
        }
    }
}