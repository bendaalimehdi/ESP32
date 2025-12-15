#pragma once
#include "ConfigLoader.h"
#include "actuators/Electrovanne.h"
#include <Arduino.h> // Pour Serial

#define MAX_VALVES 20 

class IrrigationManager {
public:
    /**
     * @brief Constructeur
     * @param logic La configuration logique (seuils, durée)
     * @param valves Un tableau de pointeurs vers les 5 objets Electrovanne
     */
    IrrigationManager(const ConfigLogic& logic, Electrovanne* valves[MAX_VALVES]);

    /**
     * @brief Traite une donnée d'humidité reçue.
     * C'est ici que la décision d'irriguer est prise.
     * @param sensorIndex L'index du capteur (1 à 5)
     * @param humidity L'humidité lue (en %)
     */
    void processSensorData(int sensorIndex, float humidity);

private:
    const ConfigLogic& _logic;
    Electrovanne* _valves[MAX_VALVES];
};