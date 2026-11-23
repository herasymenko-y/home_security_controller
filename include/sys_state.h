#ifndef SYS_STATE_H
#define SYS_STATE_H

#include <pthread.h> 
#include <semaphore.h>
#include <stdatomic.h>

#define STATE_IDLE 0 // сон
#define STATE_SCANNING 1 // скан
#define STATE_ALARM 2 // тривога
#define STATE_DISARMED 3 // зчит. картка
#define STATE_ERROR 4

extern int curr_state;

extern pthread_mutex_t sys_mut;
extern pthread_cond_t sys_sig;

extern sem_t sem_rfid;
 
extern atomic_bool is_run; 

#endif //SYS_STATE_H


