#pragma once
#include <Arduino.h>

class Electrovanne {
public:
    /**
     * @brief Constructeur.
     * @param pin Le pin GPIO qui contrôle le relais de la vanne.
     */
    Electrovanne(uint8_t pin);

    /**
     * @brief Initialise le pin en sortie.
     */
    void begin();

    /**
     * @brief Ouvre la vanne.
     * @param duration_ms Durée en millisecondes avant fermeture auto.
     * Si 0, la vanne reste ouverte indéfiniment.
     */
    void open(uint32_t duration_ms = 0);

    /**
     * @brief Ferme la vanne et annule tout minuteur.
     */
    void close();

    /**
     * @brief Fonction à appeler dans la loop() principale.
     * Gère le minuteur de fermeture automatique.
     */
    void update();

    /**
     * @brief Vérifie si la vanne est actuellement ouverte.
     */
    bool isOpen() const;

private:
    uint8_t _pin;
    bool _isOpen;
    unsigned long _autoCloseTime; // Timestamp (millis()) de fermeture
};