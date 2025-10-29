#pragma once
#include <Adafruit_NeoPixel.h> 

enum class LedState {
    IDLE,
    SEARCHING,
    CONNECTED
};

class Actuator {
public:
  
    Actuator(uint8_t pin, uint8_t brightness, float humidity_threshold);
    
    void begin();
    void update(); 
    void showSearching();
    void showConnected();
    void showStatus(float humidity); 

private:
    
    uint8_t brightness; 
    float humidity_threshold; 
    
    LedState currentState;
    unsigned long stateStartTime;
    unsigned long lastBlinkTime;
    bool blinkState;

    Adafruit_NeoPixel pixel;
    
    void setColor(uint32_t color); 
};