#include "event_queue.h"
#include "logger.h"
#include "sys_state.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

struct event_node {
    int type;
    int data;
    struct event_node *next;
};

static struct event_node *q_head = NULL;
static struct event_node *q_tail = NULL;
static pthread_mutex_t q_mut = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t q_cond = PTHREAD_COND_INITIALIZER;

static int set_system_state(int new_state, const char *state_str, const char *msg) {
    pthread_mutex_lock(&sys_mut);
    curr_state = new_state;
    pthread_cond_broadcast(&sys_sig);
    pthread_mutex_unlock(&sys_mut);

    return log_event(state_str, msg);
}

static void on_motion(int state, int data) {
    (void)data;
    if (state == STATE_IDLE) {
        if (set_system_state(STATE_SCANNING, "SCANNING", "Motion detected, waiting for RFID") != 0) {
            fprintf(stderr, "Logger write error on EVENT_MOTION\n");
        }
        sem_post(&sem_rfid);
    }
}

static void on_rfid_ok(int state, int data) {
    (void)data;
    if (state == STATE_SCANNING || state == STATE_ALARM) {
        if (set_system_state(STATE_DISARMED, "DISARMED", "Valid RFID card accepted") != 0) {
            fprintf(stderr, "Logger write error on EVENT_RFID_OK\n");
        }
    }
}

static void on_rfid_fail(int state, int data) {
    (void)data;
    if (state == STATE_SCANNING) {
        if (set_system_state(STATE_ALARM, "ALARM", "Invalid RFID card or scan timeout") != 0) {
            fprintf(stderr, "Logger write error on EVENT_RFID_FAIL\n");
        }
    }
}

static void on_env_alert(int state, int data) {
    (void)data;
    if (state != STATE_ALARM) {
        if (set_system_state(STATE_ALARM, "ALARM", "Environment sensor threshold exceeded") != 0) {
            fprintf(stderr, "Logger write error on EVENT_ENV_ALERT\n");
        }
    }
}

static void on_btn_reset(int state, int data) {
    (void)state;
    (void)data;
    if (set_system_state(STATE_IDLE, "IDLE", "System reset to IDLE by button") != 0) {
        fprintf(stderr, "Logger write error on EVENT_BTN_RESET\n");
    }
}

static void on_btn_poweroff(int state, int data) {
    (void)state;
    (void)data;
    if (log_event("SHUTDOWN", "Power-off button pressed, stopping daemon") != 0) {
        fprintf(stderr, "Logger write error on EVENT_BTN_POWEROFF\n");
    }
    is_run = false;
    pthread_mutex_lock(&sys_mut);
    pthread_cond_broadcast(&sys_sig);
    pthread_mutex_unlock(&sys_mut);
    sem_post(&sem_rfid);
}

static void on_hw_error(int state, int data) {
    (void)state;
    (void)data;
    if (set_system_state(STATE_ERROR, "ERROR", "Hardware or bus communication error") != 0) {
        fprintf(stderr, "Logger write error on EVENT_HW_ERROR\n");
    }
}

static void (*const event_handlers[])(int state, int data) = {
    [EVENT_MOTION]       = on_motion,
    [EVENT_RFID_OK]      = on_rfid_ok,
    [EVENT_RFID_FAIL]    = on_rfid_fail,
    [EVENT_ENV_ALERT]    = on_env_alert,
    [EVENT_BTN_RESET]    = on_btn_reset,
    [EVENT_BTN_POWEROFF] = on_btn_poweroff,
    [EVENT_HW_ERROR]     = on_hw_error
};

int event_queue_push(int type, int data) {
    struct event_node *node = malloc(sizeof(struct event_node));
    if (node == NULL) {
        perror("Event malloc failed");
        return -1;
    }
    node->type = type;
    node->data = data;
    node->next = NULL;

    if (pthread_mutex_lock(&q_mut) != 0) {
        free(node);
        return -1;
    }

    if (q_tail == NULL) {
        q_head = node;
        q_tail = node;
    } else {
        q_tail->next = node;
        q_tail = node;
    }

    int sig_res = pthread_cond_signal(&q_cond);
    pthread_mutex_unlock(&q_mut);
if (sig_res != 0){
return -1;
}
return 0;
}

int event_queue_stop(void) {
    if (pthread_mutex_lock(&q_mut) != 0) {
        return -1;
    }
    int res = pthread_cond_broadcast(&q_cond);
    pthread_mutex_unlock(&q_mut);
if (res != 0){
return -1;
}
    return 0;
}

static void handle_event(int type, int data) {
    int count = (int)(sizeof(event_handlers) / sizeof(event_handlers[0]));
    if (type < 0 || type >= count || event_handlers[type] == NULL) {
        fprintf(stderr, "Unknown event type: %d\n", type);
        return;
    }

    pthread_mutex_lock(&sys_mut);
    int state = curr_state;
    pthread_mutex_unlock(&sys_mut);

    event_handlers[type](state, data);
}

void *event_loop_thread(void *arg) {
    (void)arg;

    while (is_run) {
        pthread_mutex_lock(&q_mut);
        while (q_head == NULL && is_run) {
            pthread_cond_wait(&q_cond, &q_mut);
        }

        if (!is_run) {
            pthread_mutex_unlock(&q_mut);
            break;
        }

        struct event_node *node = q_head;
        q_head = node->next;
        if (q_head == NULL) {
            q_tail = NULL;
        }
        pthread_mutex_unlock(&q_mut);

        handle_event(node->type, node->data);
        free(node);
    }

    pthread_mutex_lock(&q_mut);
    while (q_head != NULL) {
        struct event_node *tmp = q_head;
        q_head = q_head->next;
        free(tmp);
    }
    q_tail = NULL;
    pthread_mutex_unlock(&q_mut);

    return NULL;
}
