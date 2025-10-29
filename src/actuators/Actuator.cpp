#include "Actuator.h"


#define COLOR_RED     0xAA0000
#define COLOR_GREEN   0x00AA00
#define COLOR_BLUE    0x0000AA
#define COLOR_OFF     0x000000


Actuator::Actuator(uint8_t pin, uint8_t brightness, float humidity_threshold) 
    : brightness(brightness), 
      humidity_threshold(humidity_threshold), 
      currentState(LedState::SEARCHING),
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
}


void Actuator::setColor(uint32_t color) {
    pixel.setPixelColor(0, color);
    pixel.show();
}

void Actuator::update() {
    unsigned long now = millis();

    switch (currentState) {
        case LedState::SEARCHING:
          
            if (now - lastBlinkTime > 100) {
                lastBlinkTime = now;
                blinkState = !blinkState;
                setColor(blinkState ? COLOR_BLUE : COLOR_OFF);
            }
            break;

        case LedState::CONNECTED:
          
            if (now - lastBlinkTime > 100) {
                lastBlinkTime = now;
                blinkState = !blinkState;
                setColor(blinkState ? COLOR_GREEN : COLOR_OFF);
            }
           
            if (now - stateStartTime > 20000) {
                currentState = LedState::IDLE;
           
            }
            break;

        case LedState::IDLE:
          
            break;
    }
}

void Actuator::showSearching() {
    currentState = LedState::SEARCHING;
    stateStartTime = millis();
}

void Actuator::showConnected() {
    if (currentState == LedState::CONNECTED) {
        stateStartTime = millis(); 
        return;
    }
    currentState = LedState::CONNECTED;
    stateStartTime = millis();
    blinkState = false; 
}

void Actuator::showStatus(float humidity) {

    if (currentState == LedState::IDLE) {
       
        if (humidity > humidity_threshold) {
            setColor(COLOR_GREEN); 
        } else {
            setColor(COLOR_RED); 
        }
    }
}