
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "sys_state.h"
#include "rfid.h"



void* rfid_thread(void* arg)
{
    int fd = *(int *)arg;

    if (fd < 0) {
        pthread_mutex_lock(&sys_mut);
        curr_state = STATE_ERROR;
        pthread_cond_broadcast(&sys_sig);
        pthread_mutex_unlock(&sys_mut);
        return NULL;
    }

    if (mfrc522_init(fd) < 0) {
        close(fd);

        pthread_mutex_lock(&sys_mut);
        curr_state = STATE_ERROR;
        pthread_cond_broadcast(&sys_sig);
        pthread_mutex_unlock(&sys_mut);

        return NULL;
    }

    while (is_run) {
        sem_wait(&sem_rfid);

        if (!is_run) {
            break;
        }

        pthread_mutex_lock(&sys_mut);

        if (curr_state == STATE_IDLE) {
            curr_state = STATE_SCANNING;
            pthread_cond_broadcast(&sys_sig);
        }

        if (curr_state != STATE_SCANNING) {
            pthread_mutex_unlock(&sys_mut);
            continue;
        }

        pthread_mutex_unlock(&sys_mut);

        int card_found = 0;
        int hardware_error = 0;

        printf("Waiting for a card...\n");

        uint8_t uid[5];

        for (int attempt = 1; attempt <= MAX_CARD_ATTEMPTS; attempt++) {

            printf("Attempt %d of %d\n", attempt, MAX_CARD_ATTEMPTS);

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

            if (attempt < MAX_CARD_ATTEMPTS) {
                printf("Card not detected. Please tap again!\n");
            }
        }

        if (card_found) {
            printf("Card detected!\n");

            pthread_mutex_lock(&sys_mut);

            if (curr_state == STATE_SCANNING) {
                curr_state = STATE_DISARMED;
                pthread_cond_broadcast(&sys_sig);
            }

            pthread_mutex_unlock(&sys_mut);

        } else if (hardware_error) {
            printf("RFID hardware error!\n");

            pthread_mutex_lock(&sys_mut);

            if (curr_state == STATE_SCANNING) {
                curr_state = STATE_ERROR;
                pthread_cond_broadcast(&sys_sig);
            }

            pthread_mutex_unlock(&sys_mut);

        } else if (is_run) {
            printf("Timeout: Card not detected!\n");

            pthread_mutex_lock(&sys_mut);

            if (curr_state == STATE_SCANNING) {
                curr_state = STATE_ALARM;
                pthread_cond_broadcast(&sys_sig);
            }

            pthread_mutex_unlock(&sys_mut);
        }
    }

    return NULL;
}

