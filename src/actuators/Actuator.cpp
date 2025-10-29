#include "Actuator.h"

// Définition des couleurs (Rouge, Vert, Bleu)
#define COLOR_RED     0xAA0000
#define COLOR_GREEN   0x00AA00
#define COLOR_BLUE    0x0000AA
#define COLOR_OFF     0x000000

Actuator::Actuator(uint8_t pin) 
    : pixel(1, pin, NEO_GRB + NEO_KHZ800),
      currentState(LedState::SEARCHING), // Démarrage en mode "recherche"
      stateStartTime(0),
      lastBlinkTime(0),
      blinkState(false) {}

void Actuator::begin() {
    pixel.begin();
    pixel.setBrightness(50); // Mettre une luminosité faible (50/255)
    pixel.show();
    stateStartTime = millis();
}

void Actuator::setColor(uint32_t color) {
    pixel.setPixelColor(0, color);
    pixel.show();
}

void Actuator::update() {
    unsigned long now = millis();

    switch (currentState) {
        case LedState::SEARCHING:
            // Clignotement bleu rapide (toutes les 100ms)
            if (now - lastBlinkTime > 100) {
                lastBlinkTime = now;
                blinkState = !blinkState;
                setColor(blinkState ? COLOR_BLUE : COLOR_OFF);
            }
            break;

        case LedState::CONNECTED:
            // Clignotement vert rapide (toutes les 100ms)
            if (now - lastBlinkTime > 100) {
                lastBlinkTime = now;
                blinkState = !blinkState;
                setColor(blinkState ? COLOR_GREEN : COLOR_OFF);
            }
            // Après 20 secondes, repasser en mode IDLE
            if (now - stateStartTime > 20000) {
                currentState = LedState::IDLE;
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
        return;
    }
    currentState = LedState::CONNECTED;
    stateStartTime = millis();
    blinkState = false; 
}

void Actuator::showStatus(float humidity) {
    // Ce statut n'est affiché que si la LED est en mode IDLE
    if (currentState == LedState::IDLE) {
        if (humidity > HUMIDITY_THRESHOLD) {
            setColor(COLOR_GREEN); // Humide = Vert
        } else {
            setColor(COLOR_RED);   // Sec = Rouge
        }
    }
}