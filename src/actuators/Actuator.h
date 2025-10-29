#pragma once
#include <Adafruit_NeoPixel.h>
#include "Config.h"

// États de la LED
enum class LedState {
    IDLE,        // Ne fait rien (sera Rouge ou Vert fixe)
    SEARCHING,   // Clignotement bleu rapide
    CONNECTED    // Clignotement vert rapide (pour 20s)
};

class Actuator {
public:
    Actuator(uint8_t pin);
    void begin();
    
    // Fonction principale à appeler dans loop()
    void update(); 

    // Commandes de haut niveau
    void showSearching();
    void showConnected();
    void showStatus(float humidity); // Affiche Rouge ou Vert

private:
    Adafruit_NeoPixel pixel;
    LedState currentState;
    
    unsigned long stateStartTime;
    unsigned long lastBlinkTime;
    bool blinkState;

    void setColor(uint32_t color);
};