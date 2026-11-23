#include <semaphore.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "sys_state.h"
#include "rfid.h"
#include "event_queue.h"
#include "logger.h"

void *rfid_thread(void *arg) {
    if (arg == NULL) {
        if (log_event("ERROR", "RFID thread: NULL argument") != 0) {
            fprintf(stderr, "Logger write error in rfid_thread\n");
        }
        return NULL;
    }

    int fd = *(int *)arg;
    if (fd < 0) {
        if (log_event("ERROR", "RFID thread: invalid SPI fd") != 0) {
            fprintf(stderr, "Logger write error in rfid_thread\n");
        }
        if (event_queue_push(EVENT_HW_ERROR, 0) != 0) {
            fprintf(stderr, "RFID: failed to push EVENT_HW_ERROR\n");
        }
        return NULL;
    }

    if (mfrc522_init(fd) < 0) {
        fprintf(stderr, "Error initializing MFRC522\n");
        if (log_event("ERROR", "Failed to initialize MFRC522") != 0) {
            fprintf(stderr, "Logger write error on MFRC522 init\n");
        }
        if (event_queue_push(EVENT_HW_ERROR, 0) != 0) {
            fprintf(stderr, "RFID: failed to push EVENT_HW_ERROR\n");
        }
        return NULL;
    }

    while (is_run) {
        sem_wait(&sem_rfid);

        if (!is_run) {
            break;
        }

        int card_found = 0;
        int hardware_error = 0;
        uint8_t uid[5];

        for (int attempt = 1; attempt <= MAX_CARD_ATTEMPTS; attempt++) {
            time_t start_time = time(NULL);

            while (time(NULL) - start_time < CARD_ATTEMPT_TIMEOUT) {
                if (!is_run) {
                    break;
                }

                int result = mfrc522_read_card(fd, uid);

                if (result == 1) {
                    card_found = 1;
                    break;
                }

                if (result < 0) {
                    hardware_error = 1;
                    break;
                }

                usleep(200000);
            }

            if (card_found || hardware_error || !is_run) {
                break;
            }
        }

        if (card_found) {
            if (event_queue_push(EVENT_RFID_OK, 0) != 0) {
                fprintf(stderr, "RFID: failed to push EVENT_RFID_OK\n");
            }
        } else if (hardware_error) {
            fprintf(stderr, "RFID hardware error\n");
            if (log_event("ERROR", "RFID: card read hardware error") != 0) {
                fprintf(stderr, "Logger write error on RFID read\n");
            }
            if (event_queue_push(EVENT_HW_ERROR, 0) != 0) {
                fprintf(stderr, "RFID: failed to push EVENT_HW_ERROR\n");
            }
        } else if (is_run) {
            if (event_queue_push(EVENT_RFID_FAIL, 0) != 0) {
                fprintf(stderr, "RFID: failed to push EVENT_RFID_FAIL\n");
            }
        }
    }

    return NULL;
}
