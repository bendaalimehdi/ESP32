#pragma once
#include <Adafruit_NeoPixel.h> // Ajout de la bibliothèque

enum class LedState {
    IDLE,
    SEARCHING,
    CONNECTED
};

class Actuator {
public:
    // Le constructeur prend le pin, la luminosité, et le seuil
    Actuator(uint8_t pin, uint8_t brightness, float humidity_threshold);
    
    void begin();
    void update(); 
    void showSearching();
    void showConnected();
    void showStatus(float humidity); // Affiche Rouge ou Vert

private:
    //uint8_t ledPin; // Supprimé, géré par l'objet pixel
    uint8_t brightness; 
    float humidity_threshold; 
    
    LedState currentState;
    unsigned long stateStartTime;
    unsigned long lastBlinkTime;
    bool blinkState;

    Adafruit_NeoPixel pixel; // Objet NeoPixel
    
    // La méthode interne utilise maintenant les codes de couleur NeoPixel (uint32_t)
    void setColor(uint32_t color); 
};