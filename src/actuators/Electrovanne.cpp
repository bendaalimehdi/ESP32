#include "Electrovanne.h"

// Suppose que HIGH = Relais ON = Vanne OUVERTE
#define VALVE_OPEN_STATE HIGH
#define VALVE_CLOSED_STATE LOW

Electrovanne::Electrovanne(uint8_t pin)
    : _pin(pin),
      _isOpen(false),
      _autoCloseTime(0) {}

void Electrovanne::begin() {
    if (_pin == 0) {
        return;
    }
    pinMode(_pin, OUTPUT);
    close(); // S'assurer que la vanne est fermée au démarrage
}

void Electrovanne::open(uint32_t duration_ms) {
    if (_pin == 0) {
        return;
    }
    digitalWrite(_pin, VALVE_OPEN_STATE);
    _isOpen = true;

    if (duration_ms > 0) {
        // Définit un temps de fermeture auto
        _autoCloseTime = millis() + duration_ms;
    } else {
        // 0 = reste ouvert
        _autoCloseTime = 0;
    }
}

void Electrovanne::close() {
    if (_pin == 0) 
    return;
    digitalWrite(_pin, VALVE_CLOSED_STATE);
    _isOpen = false;
    _autoCloseTime = 0; // Annule tout minuteur
}

bool Electrovanne::isOpen() const {
    return _isOpen;
}

void Electrovanne::update() {
    // Vérifie s'il y a un minuteur actif et s'il est temps de fermer
    if (_autoCloseTime > 0 && millis() >= _autoCloseTime) {
        close();
        Serial.print("Vanne sur pin ");
        Serial.print(_pin);
        Serial.println(" fermée par le minuteur.");
    }
}