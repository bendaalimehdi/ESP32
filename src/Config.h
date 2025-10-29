#pragma once
#include "SharedStructures.h" 

// --- Identité du Nœud (POUR JSON) ---
// MODIFIEZ CECI POUR CHAQUE NŒUD
#define FARM_ID "FARM001"
#define ZONE_ID "ZONE001"
#define NODE_ID "NODE_001" // Mettez "NODE_001" pour le follower, "MASTER_001" pour le master

// Détermine si ce nœud est le master (pour le JSON)
// Mettez 'false' pour le Follower, 'true' pour le Master
const bool IS_MASTER_NODE = true; 

// --- Buffer de communication ---
// Taille max du payload JSON (ESP-NOW est ~250 octets max)
#define MAX_PAYLOAD_SIZE 250

// --- Pins de la LED (partagés) ---
#define PIN_NEOPIXEL 48
#define RGB_BRIGHTNESS 100

// --- Pins du Capteur (Follower) ---
#define PIN_SOIL_SENSOR 2
#define PIN_SOIL_POWER 1
#define SENSOR_DRY_VALUE 3100
#define SENSOR_WET_VALUE 1200
#define PIN_DHT22 12 

// ... (Le reste de Config.h : LORA_PINS, LORA_FREQ, Adresses, etc. est inchangé) ...
const LoraPins LORA_PINS = {
    .cs = 10,
    .reset = 9,
    .irq = 2
};
const long LORA_FREQ = 868E6;
const uint8_t LORA_SYNC_WORD = 0x12;
const uint8_t masterMacAddress[] = {0x80, 0xB5, 0x4E, 0xC3, 0x23, 0xCC}; // Mettez la vraie MAC
const uint8_t LORA_MASTER_ADDR = 0x01;
const uint8_t LORA_FOLLOWER_ADDR = 0x02;
#define HUMIDITY_THRESHOLD 50.0f