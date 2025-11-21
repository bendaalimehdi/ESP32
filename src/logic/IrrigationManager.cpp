#include "IrrigationManager.h"

IrrigationManager::IrrigationManager(const ConfigLogic& logic, Electrovanne* valves[MAX_VALVES])
    : _logic(logic) 
{
    // Copie les pointeurs de vannes
    for (int i = 0; i < MAX_VALVES; i++) {
        _valves[i] = valves[i];
    }
}

void IrrigationManager::processSensorData(int sensorIndex, float humidity) {
    // Vérifie que l'index est valide (1-5)
    if (sensorIndex < 1 || sensorIndex > MAX_VALVES) {
        return;
    }
    
    // Récupère la bonne vanne (index 1 -> tableau 0)
    Electrovanne* valve = _valves[sensorIndex - 1];
    if (valve == nullptr) {
        return;
    }

    // --- CŒUR DE VOTRE LOGIQUE DE SECOURS ---

    // 1. Si l'humidité est TROP BASSE (ex: < 40%)
    if (humidity < _logic.humidity_thresholdMin) {
        // Et que la vanne n'est pas déjà ouverte...
        if (!valve->isOpen()) {
            Serial.print("[LOGIQUE SECOURS] Humidité (");
            Serial.print(humidity, 1);
            Serial.print("%) < min (");
            Serial.print(_logic.humidity_thresholdMin, 1);
            Serial.print("%). OUVERTURE vanne #");
            Serial.println(sensorIndex);
            
            // Ouvre la vanne pour la durée par défaut
            valve->open(_logic.defaultIrrigationDurationMs);
        }
    }
    // 2. Si l'humidité est TROP HAUTE (ex: > 60%)
    else if (humidity > _logic.humidity_thresholdMax) {
        // Et que la vanne est ouverte...
        if (valve->isOpen()) {
            Serial.print("[LOGIQUE SECOURS] Humidité (");
            Serial.print(humidity, 1);
            Serial.print("%) > max (");
            Serial.print(_logic.humidity_thresholdMax, 1);
            Serial.print("%). FERMETURE vanne #");
            Serial.println(sensorIndex);
            
            // Force la fermeture de la vanne
            valve->close();
        }
    }
    // 3. Si l'humidité est dans la plage (entre 40% et 60%)
    // On ne fait rien, on laisse la vanne terminer son cycle si elle est ouverte.
}