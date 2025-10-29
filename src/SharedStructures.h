#pragma once
#include <stdint.h> // Pour uint8_t

/**
 * @brief Définit les pins GPIO pour le module LoRa.
 */
struct LoraPins {
    int cs;     // Chip Select
    int reset;  // Reset
    int irq;    // DIO0 (Interrupt Request)
};

