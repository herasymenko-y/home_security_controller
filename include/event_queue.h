#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#define EVENT_MOTION       0 // зафіксовано рух
#define EVENT_RFID_OK      1 // DISARMED
#define EVENT_RFID_FAIL    2 // ALARM
#define EVENT_ENV_ALERT    3 // ALARM
#define EVENT_BTN_RESET    4 // IDLE
#define EVENT_BTN_POWEROFF 5 // зупинка демона
#define EVENT_HW_ERROR     6 // ERROR

int event_queue_push(int type, int data);
int event_queue_stop(void);
void *event_loop_thread(void *arg);

#endif // EVENT_QUEUE_H
