#include <Arduino.h>
#include "node/Master.h"
#include "node/Follower.h"

// =======================================================
//   CHOIX DU MODE
//   Mettez 1 pour flasher le FOLLOWER
//   Mettez 0 pour flasher le MASTER
// =======================================================
#define IS_FOLLOWER 0

// =======================================================


#if IS_FOLLOWER
  Follower node;
#else
  Master node;
#endif


void setup() {
    Serial.begin(115200);
    delay(1000); // Attendre que le port série soit prêt
    
    node.begin();
}

void loop() {
    node.update();
}