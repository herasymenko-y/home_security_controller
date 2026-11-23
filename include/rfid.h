#ifndef RFID_H
#define RFID_H

#include "mfrc522.h"

#define MAX_CARD_ATTEMPTS 3
#define CARD_ATTEMPT_TIMEOUT 5

void* rfid_thread(void* arg);

#endif // RFID_H
