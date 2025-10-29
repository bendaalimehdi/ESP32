#include <Arduino.h>
#include <SPIFFS.h>       // Requis pour le système de fichiers
#include "ConfigLoader.h" // Requis pour charger la config
#include "node/Master.h"
#include "node/Follower.h"

// ... (vos pointeurs globaux) ...
Config g_config;
Master* g_master = nullptr;
Follower* g_follower = nullptr;


void setup() {
    Serial.begin(115200);
    delay(2000); 
    Serial.println("Démarrage du nœud...");

    // 3. Initialiser le système de fichiers
    if (!SPIFFS.begin(true)) {
        Serial.println("Échec montage SPIFFS. Blocage.");
        while(1) delay(100);
    }

    
   
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    if (!file) {
        Serial.println("ERREUR: Le système de fichiers SPIFFS est COMPLÈTEMENT VIDE.");
    }
    while(file){
        Serial.print("Fichier trouvé: ");
        file = root.openNextFile();
    }
 

    // 4. Charger la config
    if (!loadConfig(g_config)) {
        Serial.println("Échec chargement config. Blocage.");
        while(1) delay(100); 
    }
    

    if (g_config.identity.isMaster) {
        Serial.println("Rôle (depuis config.json): MASTER. Initialisation...");
        g_master = new Master(g_config);
        g_master->begin();
    } else {
        Serial.println("Rôle (depuis config.json): FOLLOWER. Initialisation...");
        g_follower = new Follower(g_config);
        g_follower->begin();
    }
    
    Serial.println("Initialisation terminée. Démarrage de la boucle.");
}

void loop() {
    if (g_master) {
        g_master->update();
    } else if (g_follower) {
        g_follower->update();
    }
}